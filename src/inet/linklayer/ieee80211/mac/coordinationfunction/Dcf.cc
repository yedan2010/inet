//
// Copyright (C) 2016 OpenSim Ltd.
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

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Dcf.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/LegacyDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/framesequence/DcfFs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/linklayer/ieee80211/mac/lifetime/DcfTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(Dcf);

inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}

void Dcf::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        dcfChannelAccess = new DcfChannelAccess(rateSelection);
        const IIeee80211Mode *referenceMode = rateSelection->getSlowestMandatoryMode(); // or any other; slotTime etc must be the same for all modes we use
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        // rx->registerContention(contention);
    }
}

void Dcf::channelAccessGranted()
{
    FrameSequenceContext *context = nullptr;
    frameSequenceHandler->startFrameSequence(new DcfFs(), context, this);
}

void Dcf::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    if (pendingQueue->insert(frame)) {
        EV_INFO << "Frame " << frame->getName() << " has been inserted into the PendingQueue." << endl;
        dcfChannelAccess->requestChannelAccess(this);
    }
    else {
        EV_INFO << "Frame " << frame->getName() << " has been dropped because the PendingQueue is full." << endl;
        delete frame;
    }
}

void Dcf::processLowerFrame(Ieee80211Frame* frame)
{
    if (frameSequenceHandler->isSequenceRunning())
        frameSequenceHandler->processResponse(frame);
    else
        recipientProcessReceivedFrame(frame);
}

void Dcf::transmitFrame(Ieee80211Frame* frame, simtime_t ifs)
{
    tx->transmitFrame(frame, ifs, this);
}

void Dcf::frameSequenceFinished()
{
    if (hasFrameToTransmit())
        dcfChannelAccess->requestChannelAccess(this);
}

bool Dcf::isReceptionInProgress()
{
    return rx->isReceptionInProgress();
}

void Dcf::recipientProcessReceivedFrame(Ieee80211Frame* frame)
{
    recipientAckProcedure->processReceivedFrame(frame);
    auto ack = recipientAckProcedure->buildAck(frame);
    if (ack) {
        tx->transmitFrame(ack, sifs, this);
        recipientAckProcedure->processTransmittedAck(ack);
    }
    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame*>(frame)) {
        sendUp(recipientDataService->dataFrameReceived(dataFrame));
    }
    else { // TODO: else if (auto ctrlFrame = dynamic_cast<Ieee80211ControlFrame*>(frame))
        sendUp(recipientDataService->controlFrameReceived(frame));
        recipientProcessControlFrame(frame);
    }
}

void Dcf::sendUp(const std::vector<Ieee80211Frame*>& completeFrames)
{
//    for (auto frame : completeFrames)
//        mac->sendUp(frame);
}

void Dcf::recipientProcessControlFrame(Ieee80211Frame* frame)
{
    if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
        ctsProcedure->processReceivedRts(rtsFrame);
        auto ctsFrame = ctsProcedure->buildCts(rtsFrame);
        if (ctsFrame) {
            tx->transmitFrame(ctsFrame, sifs, this);
            ctsProcedure->processTransmittedCts(ctsFrame);
        }
    }
}

void Dcf::transmissionComplete()
{
    if (frameSequenceHandler->isSequenceRunning())
        frameSequenceHandler->transmissionComplete();
    else ;
}

bool Dcf::hasFrameToTransmit()
{
    return !pendingQueue->isEmpty() || inProgressFrames->hasInProgressFrames();
}

void Dcf::originatorProcessRtsProtectionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    EV_INFO << "RTS frame transmission failed\n";
    recoveryProcedure->rtsFrameTransmissionFailed(protectedFrame);
    bool retryLimitReached = recoveryProcedure->isRtsFrameRetryLimitReached(protectedFrame);
    if (retryLimitReached) {
        inProgressFrames->dropFrame(protectedFrame);
        delete protectedFrame;
    }
}

void Dcf::originatorProcessTransmittedFrame(Ieee80211Frame* transmittedFrame)
{
    if (transmittedFrame->getReceiverAddress().isMulticast())
        recoveryProcedure->multicastFrameTransmitted();
    if (transmittedFrame->getType() == ST_DATA || dynamic_cast<Ieee80211ManagementFrame *>(transmittedFrame))
        ackHandler->processTransmittedNonQoSFrame(check_and_cast<Ieee80211DataOrMgmtFrame *>(transmittedFrame));
    else
        ;
}

void Dcf::originatorProcessReceivedFrame(Ieee80211Frame* frame, Ieee80211Frame* lastTransmittedFrame)
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

void Dcf::originatorProcessFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame)
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

} // namespace ieee80211
} // namespace inet

