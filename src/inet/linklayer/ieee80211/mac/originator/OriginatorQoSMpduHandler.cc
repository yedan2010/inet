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

#include "inet/physicallayer/ieee80211/mode/Ieee80211DSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HRDSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HTMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211OFDMMode.h"
#include "OriginatorQoSMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(OriginatorQoSMpduHandler);

void OriginatorQoSMpduHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        ac = AccessCategory(getAccessCategory(par("accessCategory")));
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        modeSet = rateSelection->getModeSet();
        dataService = check_and_cast<OriginatorQoSMacDataService *>(getModuleByPath(par("originatorMacDataServiceModule")));
        pendingQueue = new PendingQueue(par("maxQueueSize"), nullptr);
        recoveryProcedure = check_and_cast<RecoveryProcedure *>(getSubmodule("recoveryProcedure"));
        ackHandler = new AckHandler();
        originatorBlockAckAgreementHandler = check_and_cast<OriginatorBlockAckAgreementHandler *>(getModuleByPath(par("originatorBlockAckAgreementHandlerModule")));
        lifetimeHandler = new EdcaTransmitLifetimeHandler(0, 0, 0, 0); // FIXME: needs only one timeout parameter
        inProgressFrames = new InProgressFrames(pendingQueue, dataService, ackHandler);
        ackProcedure = new OriginatorAckProcedure(rateSelection);
        rtsProcedure = new RtsProcedure(rateSelection);
        blockAckProcedure = new OriginatorBlockAckProcedure(rateSelection);
        simtime_t txopLimit = par("txopLimit");
        txopProcedure = new TxOpProcedure(TxOpProcedure::Type::EDCA, txopLimit != -1 ? txopLimit : getTxopLimit(rateSelection->getSlowestMandatoryMode()).get());
    }
}

void OriginatorQoSMpduHandler::processRtsProtectionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    EV_INFO << "RTS frame transmission failed\n";
    recoveryProcedure->rtsFrameTransmissionFailed(protectedFrame);
    bool retryLimitReached = recoveryProcedure->isRtsFrameRetryLimitReached(protectedFrame);
    if (retryLimitReached)
        inProgressFrames->dropAndDeleteFrame(protectedFrame);
}

void OriginatorQoSMpduHandler::processTransmittedFrame(Ieee80211Frame* transmittedFrame)
{
    if (false) // TODO: multicast...
        recoveryProcedure->multicastFrameTransmitted();
    if (transmittedFrame->getType() == ST_DATA_WITH_QOS) {
        auto dataFrame = check_and_cast<Ieee80211DataFrame *>(transmittedFrame);
        ackHandler->processTransmittedQoSData(dataFrame);
        if (dataFrame->getAckPolicy() == NO_ACK)
            inProgressFrames->dropAndDeleteFrame(dataFrame);
    }
    else if (auto blockAckReq = dynamic_cast<Ieee80211BlockAckReq*>(transmittedFrame))
        ackHandler->processTransmittedBlockAckReq(blockAckReq);
    else if (auto addbaReq = dynamic_cast<Ieee80211AddbaRequest*>(transmittedFrame)) {
        ackHandler->processTransmittedMgmtFrame(addbaReq);
    }
    else if (auto addbaResp = dynamic_cast<Ieee80211AddbaResponse*>(transmittedFrame)) {
        ackHandler->processTransmittedMgmtFrame(addbaResp);
    }
    else
        // TODO: control frames, etc, void
        ;
}

void OriginatorQoSMpduHandler::processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame)
{
    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(failedFrame)) {
        ASSERT(dataFrame->getType() == ST_DATA_WITH_QOS);
        EV_INFO << "QoS Data frame transmission failed\n";
        if (dataFrame->getAckPolicy() == NORMAL_ACK) {
            recoveryProcedure->dataOrMgmtFrameTransmissionFailed(dataFrame);
            bool retryLimitReached = recoveryProcedure->isDataOrMgtmFrameRetryLimitReached(dataFrame);
            if (retryLimitReached)
                inProgressFrames->dropAndDeleteFrame(dataFrame);
        }
        else if (dataFrame->getAckPolicy() == BLOCK_ACK) {
    // TODO:
    //        bool lifetimeExpired = lifetimeHandler->isLifetimeExpired(failedFrame);
    //        if (lifetimeExpired)
    //            inProgressFrames->dropAndDeleteFrame(failedFrame);
        }
        else
            throw cRuntimeError("Unimplemented!");
    }
    else {
        throw cRuntimeError("Unimplemented"); // failed block ack req, addba req
    }
}

void OriginatorQoSMpduHandler::processReceivedFrame(Ieee80211Frame* frame, Ieee80211Frame* lastTransmittedFrame)
{
    if (frame->getType() == ST_ACK) {
        recoveryProcedure->ackFrameReceived(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
        ackHandler->processReceivedAck(check_and_cast<Ieee80211ACKFrame *>(frame), check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
        inProgressFrames->dropAndDeleteFrame(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
    }
    else if (frame->getType() == ST_BLOCKACK) {
        EV_INFO << "BlockAck has arrived" << std::endl;
        recoveryProcedure->blockAckFrameReceived();
        auto ackedSeqAndFragNums = ackHandler->processReceivedBlockAck(check_and_cast<Ieee80211BlockAck *>(frame));
        EV_INFO << "It has acknowledged the following frames:" << std::endl;
        for (auto seqCtrlField : ackedSeqAndFragNums)
            EV_INFO << "Fragment number = " << seqCtrlField.getSequenceNumber() << " Sequence number = " << (int)seqCtrlField.getFragmentNumber() << std::endl;
        inProgressFrames->dropAndDeleteFrames(ackedSeqAndFragNums);
    }
    else if (auto addbaResponse = dynamic_cast<Ieee80211AddbaResponse*>(frame)) {
        recipientBlockAckAgreementHandler->processTransmittedAddbaResponse(addbaResponse);
    }
    else if (frame->getType() == ST_RTS)
        ; // void
    else if (frame->getType() == ST_CTS)
        recoveryProcedure->ctsFrameReceived();
    else if (frame->getType() == ST_DATA_WITH_QOS)
        ; // void
    else if (frame->getType() == ST_BLOCKACK_REQ)
        ; // void
    else
        throw cRuntimeError("Unknown frame type");
}

bool OriginatorQoSMpduHandler::internalCollision() // TODO: rename or split or move or what?
{
    Ieee80211DataOrMgmtFrame *internallyCollidedFrame = inProgressFrames->getFrameToTransmit();
    recoveryProcedure->dataOrMgmtFrameTransmissionFailed(internallyCollidedFrame);
    if (recoveryProcedure->isDataOrMgtmFrameRetryLimitReached(internallyCollidedFrame)) {
        inProgressFrames->dropAndDeleteFrame(internallyCollidedFrame);
        return false;
    }
    return true;
}


s OriginatorQoSMpduHandler::getTxopLimit(const IIeee80211Mode *mode)
{
    switch (ac)
    {
        case AC_BK: return s(0);
        case AC_BE: return s(0);
        case AC_VI:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(6.016);
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(3.008);
            else return s(0);
        case AC_VO:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(3.264);
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(1.504);
            else return s(0);
        default: throw cRuntimeError("Unknown access category = %d", ac);
    }
}

void OriginatorQoSMpduHandler::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    if (pendingQueue->insert(frame))
        EV_INFO << "Frame " << frame->getName() << " has been inserted into the PendingQueue." << endl;
    else {
        EV_INFO << "Frame " << frame->getName() << " has been dropped because the PendingQueue is full." << endl;
        delete frame;
    }
}

void OriginatorQoSMpduHandler::processBlockAckAgreementTerminated(OriginatorBlockAckAgreement* agreement)
{
}

bool OriginatorQoSMpduHandler::hasFrameToTransmit()
{
    return !pendingQueue->isEmpty() || inProgressFrames->hasInProgressFrames();
}

FrameSequenceContext* OriginatorQoSMpduHandler::buildContext()
{
    return new FrameSequenceContext(pendingQueue, inProgressFrames, ackProcedure, rtsProcedure, txopProcedure, blockAckProcedure, originatorBlockAckAgreementHandler, modeSet);
}

AccessCategory OriginatorQoSMpduHandler::getAccessCategory(const char *ac)
{
    if (strcmp("AC_BK", ac) == 0)
        return AC_BK;
    if (strcmp("AC_VI", ac) == 0)
        return AC_VI;
    if (strcmp("AC_VO", ac) == 0)
        return AC_VO;
    if (strcmp("AC_BE", ac) == 0)
        return AC_BE;
    throw cRuntimeError("Unknown Access Category = %s", ac);
}

} /* namespace ieee80211 */
} /* namespace inet */
