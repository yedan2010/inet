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

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"
#include "inet/linklayer/ieee80211/mac/framesequence/HcfFs.h"

namespace inet {
namespace ieee80211 {

Define_Module(Hcf);

void Hcf::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        int numOfEdcafs = par("numOfEdcafs");
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        for (int ac = 0; ac < numOfEdcafs; ac++) {
            originatorDataService = check_and_cast<OriginatorQoSMacDataService *>(getModuleByPath(par("originatorMacDataServiceModule")));
            recipientDataService = check_and_cast<RecipientQoSMacDataService*>(getModuleByPath(par("recipientMacDataServiceModule")));
            pendingQueues.push_back(new PendingQueue(par("maxQueueSize"), nullptr));
            recoveryProcedures.push_back(check_and_cast<RecoveryProcedure *>(getSubmodule("recoveryProcedures", ac)));
            ackHandlers.push_back(new AckHandler());
            originatorBlockAckAgreementHandler = check_and_cast<OriginatorBlockAckAgreementHandler *>(getModuleByPath(par("originatorBlockAckAgreementHandlerModule")));
            recipientBlockAckAgreementHandler = check_and_cast<RecipientBlockAckAgreementHandler*>(getModuleByPath(par("recipientBlockAckAgreementHandler")));
            lifetimeHandler = new EdcaTransmitLifetimeHandler(0, 0, 0, 0); // FIXME: needs only one timeout parameter
            inProgressFrames.push_back(new InProgressFrames(pendingQueues[ac], originatorDataService, ackHandlers[ac]));
            originatorAckProcedure = new OriginatorAckProcedure(rateSelection);
            rtsProcedure = new RtsProcedure(rateSelection);
            originatorBlockAckProcedure = new OriginatorBlockAckProcedure(rateSelection);
            recipientBlockAckProcedure = new BlockAckProcedure(recipientBlockAckAgreementHandler, rateSelection);
            recipientAckProcedure = new AckProcedure(rateSelection);
            ctsProcedure = new CtsProcedure(rx, rateSelection);
            //txopProcedure = new TxOpProcedure(TxOpProcedure::Type::EDCA, txopLimit != -1 ? txopLimit : getTxopLimit(rateSelection->getSlowestMandatoryMode()).get());
        }
    }
}

void Hcf::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    AccessCategory ac = edca->classifyFrame(frame);
    if (pendingQueues[ac]->insert(frame)) {
        EV_INFO << "Frame " << frame->getName() << " has been inserted into the PendingQueue." << endl;
        edca->requestChannelAccess(ac, this);
    }
    else {
        EV_INFO << "Frame " << frame->getName() << " has been dropped because the PendingQueue is full." << endl;
        delete frame;
    }
}

void Hcf::processLowerFrame(Ieee80211Frame* frame)
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf)
        frameSequenceHandler->processResponse(frame);
    else if (hcca->isOwning())
        throw cRuntimeError("Hcca is unimplemented!");
    else
        recipientProcessReceivedFrame(frame);
}

//FrameSequenceContext* OriginatorQoSMpduHandler::buildContext()
//{
//    return new FrameSequenceContext(pendingQueue, inProgressFrames, ackProcedure, rtsProcedure, txopProcedure, blockAckProcedure, originatorBlockAckAgreementHandler, modeSet);
//}

//s OriginatorQoSMpduHandler::getTxopLimit(const IIeee80211Mode *mode)
//{
//    switch (ac)
//    {
//        case AC_BK: return s(0);
//        case AC_BE: return s(0);
//        case AC_VI:
//            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(6.016);
//            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(3.008);
//            else return s(0);
//        case AC_VO:
//            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(3.264);
//            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(1.504);
//            else return s(0);
//        default: throw cRuntimeError("Unknown access category = %d", ac);
//    }
//}

void Hcf::startFrameSequence(AccessCategory ac)
{
    FrameSequenceContext *context = nullptr;
    frameSequenceHandler->startFrameSequence(new HcfFs(), context, this);
}

void Hcf::channelAccessGranted()
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        startFrameSequence(edcaf->getAccessCategory());
        handleInternalCollision(edca->getInternallyCollidedEdcafs());
    }
    else if (hcca->isOwning())
        throw cRuntimeError("Hcca is unimplemented!");
    else
        throw cRuntimeError("Channel access granted but channel owner not found!");
}

void Hcf::handleInternalCollision(std::vector<Edcaf*> internallyCollidedEdcafs)
{
    for (auto edcaf : internallyCollidedEdcafs) {
        AccessCategory ac = edcaf->getAccessCategory();
        Ieee80211DataOrMgmtFrame *internallyCollidedFrame = inProgressFrames[ac]->getFrameToTransmit();
        recoveryProcedures[ac]->dataOrMgmtFrameTransmissionFailed(internallyCollidedFrame);
        if (recoveryProcedures[ac]->isDataOrMgtmFrameRetryLimitReached(internallyCollidedFrame))
            inProgressFrames[ac]->dropFrame(internallyCollidedFrame);
        else
            edcaf->requestChannelAccess(this);
    }
}

void Hcf::frameSequenceFinished()
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf)
        edcaf->releaseChannelAccess(this);
    else if (hcca->isOwning())
        hcca->releaseChannelAccess(this);
    else
        throw cRuntimeError("Frame sequence finished but channel owner not found!");
}

bool Hcf::isReceptionInProgress()
{
    return rx->isReceptionInProgress();
}

void Hcf::transmitFrame(Ieee80211Frame* frame, simtime_t ifs)
{
    tx->transmitFrame(frame, ifs, this);
}

void Hcf::recipientProcessControlFrame(Ieee80211Frame* frame)
{
    if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
        ctsProcedure->processReceivedRts(rtsFrame);
        auto ctsFrame = ctsProcedure->buildCts(rtsFrame);
        if (ctsFrame) {
            tx->transmitFrame(ctsFrame, sifs, this);
            ctsProcedure->processTransmittedCts(ctsFrame);
        }
    }
    else if (auto blockAckRequest = dynamic_cast<Ieee80211BlockAckReq*>(frame)) {
        recipientBlockAckProcedure->processReceivedBlockAckReq(blockAckRequest);
        auto blockAck = recipientBlockAckProcedure->buildBlockAck(blockAckRequest);
        if (blockAck) {
            tx->transmitFrame(blockAck, sifs, this);
            recipientBlockAckProcedure->processTransmittedBlockAck(blockAck);
        }
    }
    else
        throw cRuntimeError("Unknown packet");
}

void Hcf::recipientProcessManagementFrame(Ieee80211ManagementFrame* frame)
{
    if (auto addbaRequest = dynamic_cast<Ieee80211AddbaRequest *>(frame)) {
        recipientBlockAckAgreementHandler->processReceivedAddbaRequest(addbaRequest);
        auto addbaResponse = recipientBlockAckAgreementHandler->buildAddbaResponse(addbaRequest);
        if (addbaResponse)
            processUpperFrame(addbaResponse);
    }
    else if (auto addbaResponse = dynamic_cast<Ieee80211AddbaResponse *>(frame)) {
        originatorBlockAckAgreementHandler->processReceivedAddbaResponse(addbaResponse);
    }
    else if (auto delba = dynamic_cast<Ieee80211Delba*>(frame)) {
        if (delba->getInitiator())
            recipientBlockAckAgreementHandler->processReceivedDelba(delba);
        else
            originatorBlockAckAgreementHandler->processReceivedDelba(delba);
    }
    else
        throw cRuntimeError("Unknown packet");
}

void Hcf::recipientProcessReceivedFrame(Ieee80211Frame* frame)
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
    else if (auto mgmtFrame = dynamic_cast<Ieee80211ManagementFrame*>(frame)) {
        sendUp(recipientDataService->managementFrameReceived(mgmtFrame));
        recipientProcessManagementFrame(mgmtFrame);
    }
    else { // TODO: else if (auto ctrlFrame = dynamic_cast<Ieee80211ControlFrame*>(frame))
        sendUp(recipientDataService->controlFrameReceived(frame));
        recipientProcessControlFrame(frame);
    }
}

void Hcf::transmissionComplete()
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf)
        frameSequenceHandler->transmissionComplete();
    else if (hcca->isOwning())
        throw cRuntimeError("Hcca is unimplemented!");
    else
        ;
}

void Hcf::originatorProcessRtsProtectionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        EV_INFO << "RTS frame transmission failed\n";
        AccessCategory ac = edcaf->getAccessCategory();
        recoveryProcedures[ac]->rtsFrameTransmissionFailed(protectedFrame);
        bool retryLimitReached = recoveryProcedures[ac]->isRtsFrameRetryLimitReached(protectedFrame);
        if (retryLimitReached) {
            inProgressFrames[ac]->dropFrame(protectedFrame);
            delete protectedFrame;
        }
    }
    else
        throw cRuntimeError("Hcca is unimplemented!");
}

void Hcf::originatorProcessTransmittedFrame(Ieee80211Frame* transmittedFrame)
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        AccessCategory ac = edcaf->getAccessCategory();
        if (transmittedFrame->getReceiverAddress().isMulticast())
            recoveryProcedures[ac]->multicastFrameTransmitted();
        if (transmittedFrame->getType() == ST_DATA_WITH_QOS) {
            auto dataFrame = check_and_cast<Ieee80211DataFrame *>(transmittedFrame);
            ackHandlers[ac]->processTransmittedQoSData(dataFrame);
            if (dataFrame->getAckPolicy() == NO_ACK)
                inProgressFrames[ac]->dropFrame(dataFrame);
        }
        else if (auto blockAckReq = dynamic_cast<Ieee80211BlockAckReq*>(transmittedFrame))
            ackHandlers[ac]->processTransmittedBlockAckReq(blockAckReq);
        else if (auto addbaReq = dynamic_cast<Ieee80211AddbaRequest*>(transmittedFrame)) {
            ackHandlers[ac]->processTransmittedMgmtFrame(addbaReq);
        }
        else if (auto addbaResp = dynamic_cast<Ieee80211AddbaResponse*>(transmittedFrame)) {
            ackHandlers[ac]->processTransmittedMgmtFrame(addbaResp);
        }
        else if (auto delba = dynamic_cast<Ieee80211Delba *>(transmittedFrame)) {
            ackHandlers[ac]->processTransmittedMgmtFrame(delba);
        }
        else ; // etc??
    }
    else
        throw cRuntimeError("Hcca is unimplemented!");
}

void Hcf::originatorProcessFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame)
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        AccessCategory ac = edcaf->getAccessCategory();
        if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(failedFrame)) {
            ASSERT(dataFrame->getType() == ST_DATA_WITH_QOS);
            EV_INFO << "QoS Data frame transmission failed\n";
            if (dataFrame->getAckPolicy() == NORMAL_ACK) {
                recoveryProcedures[ac]->dataOrMgmtFrameTransmissionFailed(dataFrame);
                bool retryLimitReached = recoveryProcedures[ac]->isDataOrMgtmFrameRetryLimitReached(dataFrame);
                if (retryLimitReached) {
                    inProgressFrames[ac]->dropFrame(dataFrame);
                    delete dataFrame;
                }
            }
            else if (dataFrame->getAckPolicy() == BLOCK_ACK) {
                // TODO:
                // bool lifetimeExpired = lifetimeHandler->isLifetimeExpired(failedFrame);
                // if (lifetimeExpired) {
                //    inProgressFrames->dropFrame(failedFrame);
                //    delete dataFrame;
                // }
            }
            else
                throw cRuntimeError("Unimplemented!");
        }
        else {
            throw cRuntimeError("Unimplemented"); // failed block ack req, addba req
        }
    }
    else
        throw cRuntimeError("Hcca is unimplemented!");
}

void Hcf::originatorProcessReceivedFrame(Ieee80211Frame* frame, Ieee80211Frame* lastTransmittedFrame)
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        AccessCategory ac = edcaf->getAccessCategory();
        if (frame->getType() == ST_ACK) {
            recoveryProcedures[ac]->ackFrameReceived(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
            ackHandlers[ac]->processReceivedAck(check_and_cast<Ieee80211ACKFrame *>(frame), check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
            inProgressFrames[ac]->dropFrame(check_and_cast<Ieee80211DataOrMgmtFrame*>(lastTransmittedFrame));
        }
        else if (frame->getType() == ST_BLOCKACK) {
            EV_INFO << "BlockAck has arrived" << std::endl;
            recoveryProcedures[ac]->blockAckFrameReceived();
            auto ackedSeqAndFragNums = ackHandlers[ac]->processReceivedBlockAck(check_and_cast<Ieee80211BlockAck *>(frame));
            EV_INFO << "It has acknowledged the following frames:" << std::endl;
            for (auto seqCtrlField : ackedSeqAndFragNums)
                EV_INFO << "Fragment number = " << seqCtrlField.getSequenceNumber() << " Sequence number = " << (int)seqCtrlField.getFragmentNumber() << std::endl;
            inProgressFrames[ac]->dropFrames(ackedSeqAndFragNums);
        }
        else if (auto addbaResponse = dynamic_cast<Ieee80211AddbaResponse*>(frame)) {
            recipientBlockAckAgreementHandler->processTransmittedAddbaResponse(addbaResponse);
        }
        else if (frame->getType() == ST_RTS)
            ; // void
        else if (frame->getType() == ST_CTS)
            recoveryProcedures[ac]->ctsFrameReceived();
        else if (frame->getType() == ST_DATA_WITH_QOS)
            ; // void
        else if (frame->getType() == ST_BLOCKACK_REQ)
            ; // void
        else
            throw cRuntimeError("Unknown frame type");
    }
    else
        throw cRuntimeError("Hcca is unimplemented!");
}

bool Hcf::hasFrameToTransmit()
{
    auto edcaf = edca->getChannelOwner();
    if (edcaf) {
        AccessCategory ac = edcaf->getAccessCategory();
        return !pendingQueues[ac]->isEmpty() || inProgressFrames[ac]->hasInProgressFrames();
    }
    else
        throw cRuntimeError("Hcca is unimplemented");
}

void Hcf::sendUp(const std::vector<Ieee80211Frame*>& completeFrames)
{
//    for (auto frame : completeFrames) {
//        // FIXME: mgmt module does not handle addba req ..
//        if (!dynamic_cast<Ieee80211AddbaRequest*>(frame) && !dynamic_cast<Ieee80211AddbaResponse*>(frame))
//            mac->sendUp(frame);
//    }
}


} // namespace ieee80211
} // namespace inet

