//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "OriginatorMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(OriginatorMpduHandler);

void OriginatorMpduHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        pendingQueue = new PendingQueue(par("maxQueueSize"), nullptr);
        dataService = check_and_cast<OriginatorMacDataService*>(getModuleByPath(par("originatorMacDataServiceModule")));
        ackHandler = new AckHandler();
        recoveryProcedure = check_and_cast<RecoveryProcedure *>(getSubmodule("recoveryProcedure"));
        lifetimeHandler = new DcfTransmitLifetimeHandler(0);
        inProgressFrames = new InProgressFrames(pendingQueue, dataService, ackHandler);
        ackProcedure = new OriginatorAckProcedure(rateSelection);
        rtsProcedure = new RtsProcedure(rateSelection);
    }
}

void OriginatorMpduHandler::processRtsProtectionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    EV_INFO << "RTS frame transmission failed\n";
    recoveryProcedure->rtsFrameTransmissionFailed(protectedFrame);
    bool retryLimitReached = recoveryProcedure->isRtsFrameRetryLimitReached(protectedFrame);
    if (retryLimitReached) {
        inProgressFrames->dropFrame(protectedFrame);
        delete protectedFrame;
    }
}

void OriginatorMpduHandler::processTransmittedFrame(Ieee80211Frame* transmittedFrame)
{
    if (transmittedFrame->getReceiverAddress().isMulticast())
        recoveryProcedure->multicastFrameTransmitted();
    if (transmittedFrame->getType() == ST_DATA || dynamic_cast<Ieee80211ManagementFrame *>(transmittedFrame))
        ackHandler->processTransmittedNonQoSFrame(check_and_cast<Ieee80211DataOrMgmtFrame *>(transmittedFrame));
    else
        ;
}

void OriginatorMpduHandler::processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame)
{
    ASSERT(failedFrame->getType() != ST_DATA_WITH_QOS);
    ASSERT(ackHandler->getAckStatus(failedFrame) == AckHandler::Status::WAITING_FOR_NORMAL_ACK);
    EV_INFO << "Data/Mgmt frame transmission failed\n";
    recoveryProcedure->dataOrMgmtFrameTransmissionFailed(failedFrame);
    bool retryLimitReached = recoveryProcedure->isDataOrMgtmFrameRetryLimitReached(failedFrame);
    if (retryLimitReached) {
        inProgressFrames->dropFrame(failedFrame);
        delete failedFrame;
    }
}

void OriginatorMpduHandler::processReceivedFrame(Ieee80211Frame* frame, Ieee80211Frame *lastTransmittedFrame)
{
    if (frame->getType() == ST_ACK) {
        recoveryProcedure->ackFrameReceived(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
        ackHandler->processReceivedAck(check_and_cast<Ieee80211ACKFrame *>(frame), check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
        inProgressFrames->dropFrame(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
    }
    else if (frame->getType() == ST_RTS)
        ; // void
    else if (frame->getType() == ST_CTS)
        recoveryProcedure->ctsFrameReceived();
    else
        throw cRuntimeError("Unknown frame type");
}


void OriginatorMpduHandler::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    if (pendingQueue->insert(frame))
        EV_INFO << "Frame " << frame->getName() << " has been inserted into the PendingQueue." << endl;
    else {
        EV_INFO << "Frame " << frame->getName() << " has been dropped because the PendingQueue is full." << endl;
        delete frame;
    }
}

bool OriginatorMpduHandler::hasFrameToTransmit()
{
    return !pendingQueue->isEmpty() || inProgressFrames->hasInProgressFrames();
}

} /* namespace ieee80211 */
} /* namespace inet */
