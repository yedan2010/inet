//
// Copyright (C) 2015 Andras Varga
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
//

#include "RecipientBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/blockack/BlockAckAgreement.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Edca.h"

namespace inet {
namespace ieee80211 {

Define_Module(RecipientBlockAckAgreementHandler);

void RecipientBlockAckAgreementHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        rateSelection = dynamic_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
        isDelayedBlockAckPolicySupported = par("delayedAckPolicySupported");
        isAMsduSupported = par("aMsduSupported");
        maximumAllowedBufferSize = par("maximumAllowedBufferSize");
        blockAckTimeoutValue = par("blockAckTimeoutValue").doubleValue();
    }
}

void RecipientBlockAckAgreementHandler::handleMessage(cMessage* msg)
{
    // The Block Ack Timeout Value field contains the duration, in TUs, after which the Block Ack setup is
    // terminated, if there are no frame exchanges (see 10.5.4) within this duration using this Block Ack
    // agreement. A value of 0 disables the timeout.
    for (auto it : blockAckAgreements) {
        auto *agreement = it.second;
        if (agreement->getInactivityTimer() == msg) {
            auto edca = check_and_cast<Edca*>(getParentModule()); // FIXME: khm
            Tid tid = it.first.second;
            MACAddress receiverAddr = it.first.first;
            edca->upperFrameReceived(buildDelba(receiverAddr, tid, 39)); // FIXME: 39 - TIMEOUT see: Table 8-36—Reason codes
            terminateAgreement(agreement->getBlockAckRecord()->getOriginatorAddress(), agreement->getBlockAckRecord()->getTid());
            return;
        }
    }
    throw cRuntimeError("Unknown message = %s", msg->getName());
}

//
// When a timeout of BlockAckTimeout is detected, the STA shall send a DELBA frame to the peer STA with the Reason Code
// field set to TIMEOUT and shall issue a MLME-DELBA.indication primitive with the ReasonCode
// parameter having a value of TIMEOUT. The procedure is illustrated in Figure 10-14.
//
Ieee80211Delba* RecipientBlockAckAgreementHandler::buildDelba(MACAddress receiverAddr, Tid tid, int reasonCode)
{
    Ieee80211Delba *delba = new Ieee80211Delba("Delba");
    delba->setReceiverAddress(receiverAddr);
    delba->setInitiator(false);
    delba->setTid(tid);
    delba->setReasonCode(reasonCode);
    delba->setDuration(rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK) + sifs);
    return delba;
}

Ieee80211AddbaResponse* RecipientBlockAckAgreementHandler::buildAddbaResponse(Ieee80211AddbaRequest* frame)
{
    Ieee80211AddbaResponse *addbaResponse = new Ieee80211AddbaResponse("AddbaResponse");
    addbaResponse->setReceiverAddress(frame->getTransmitterAddress());
    // The Block Ack Policy subfield is set to 1 for immediate Block Ack and 0 for delayed Block Ack.
    Tid tid = frame->getTid();
    addbaResponse->setTid(tid);
    addbaResponse->setBlockAckPolicy(!frame->getBlockAckPolicy() && isDelayedBlockAckPolicySupported ? false : true);
    addbaResponse->setBufferSize(frame->getBufferSize() <= maximumAllowedBufferSize ? frame->getBufferSize() : maximumAllowedBufferSize);
    addbaResponse->setBlockAckTimeoutValue(blockAckTimeoutValue == 0 ? blockAckTimeoutValue : frame->getBlockAckTimeoutValue());
    addbaResponse->setAMsduSupported(isAMsduSupported);
    addbaResponse->setDuration(rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK) + sifs);
    return addbaResponse;
}


void RecipientBlockAckAgreementHandler::processTransmittedAddbaResponse(Ieee80211AddbaResponse *frame)
{
    auto id = std::make_pair(frame->getReceiverAddress(), frame->getTid());
    auto it = blockAckAgreements.find(id);
    if (it != blockAckAgreements.end()) {
        BlockAckAgreement *agreement = it->second;
        agreement->addbaResposneSent();
        blockAckReordering->agreementEstablished(agreement);
    }
    else {
        throw cRuntimeError("Agreement is not found");
    }
}

//
// An originator that intends to use the Block Ack mechanism for the transmission of QoS data frames to an
// intended recipient should first check whether the intended recipient STA is capable of participating in Block
// Ack mechanism by discovering and examining its Delayed Block Ack and Immediate Block Ack capability
// bits. If the intended recipient STA is capable of participating, the originator sends an ADDBA Request frame
// indicating the TID for which the Block Ack is being set up.
//
void RecipientBlockAckAgreementHandler::processReceivedAddbaRequest(Ieee80211AddbaRequest *frame)
{
    MACAddress originatorAddr = frame->getTransmitterAddress();
    auto id = std::make_pair(originatorAddr, frame->getTid());
    auto it = blockAckAgreements.find(id);
    if (it == blockAckAgreements.end()) {
        BlockAckAgreement *agreement = new BlockAckAgreement(this, originatorAddr, frame->getTid(), frame->getStartingSequenceNumber(), frame->getBufferSize(), frame->getBlockAckTimeoutValue());
        blockAckAgreements[id] = agreement;
    }
    else
        throw cRuntimeError("TODO"); // TODO: update?
}

//
// 9.21.5 Teardown of the Block Ack mechanism
// When the originator has no data to send and the final Block Ack exchange has completed, it shall signal the end
// of its use of the Block Ack mechanism by sending the DELBA frame to its recipient. There is no management
// response frame from the recipient. The recipient of the DELBA frame shall release all resources allocated for
// the Block Ack transfer.
//
void RecipientBlockAckAgreementHandler::processReceivedDelba(Ieee80211Delba *frame)
{
    BlockAckAgreement *agreement  = terminateAgreement(frame->getTransmitterAddress(), frame->getTid());
    blockAckReordering->agreementTerminated(agreement);
    delete agreement;
}


BlockAckAgreement *RecipientBlockAckAgreementHandler::terminateAgreement(MACAddress originatorAddr, Tid tid)
{
    auto agreementId = std::make_pair(originatorAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    if (it != blockAckAgreements.end()) {
        BlockAckAgreement *agreement = it->second;
        blockAckAgreements.erase(it);
        return agreement;
    }
    else
        throw cRuntimeError("BA Agreement entry is not found");
}

BlockAckAgreement* RecipientBlockAckAgreementHandler::getAgreement(Tid tid, MACAddress originatorAddr)
{
    auto agreementId = std::make_pair(originatorAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    return it != blockAckAgreements.end() ? it->second : nullptr;
}

} // namespace ieee80211
} // namespace inet
