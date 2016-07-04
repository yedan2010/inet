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
// Author: Andras Varga
//

#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "OriginatorBlockAckAgreementHandler.h"


namespace inet {
namespace ieee80211 {

Define_Module(OriginatorBlockAckAgreementHandler);

void OriginatorBlockAckAgreementHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        rateSelection = dynamic_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
        slotTime = rateSelection->getSlowestMandatoryMode()->getSlotTime();
        phyRxStartDelay = rateSelection->getSlowestMandatoryMode()->getPhyRxStartDelay();
        isDelayedBlockAckPolicySupported = par("delayedAckPolicySupported");
        isAMsduSupported = par("aMsduSupported");
        maximumAllowedBufferSize = par("maximumAllowedBufferSize");
        blockAckTimeoutValue = par("blockAckTimeoutValue").doubleValue();
    }
}

void OriginatorBlockAckAgreementHandler::handleMessage(cMessage* msg)
{
    throw cRuntimeError("This module does not handle self messages.");
}

void OriginatorBlockAckAgreementHandler::processAddbaRequest(Ieee80211AddbaRequest *addbaRequest)
{
    OriginatorBlockAckAgreement *blockAckAgreement = new OriginatorBlockAckAgreement(addbaRequest->getBufferSize(), addbaRequest->getAMsduSupported(), addbaRequest->getBlockAckPolicy() == 0);
    auto agreementId = std::make_pair(addbaRequest->getReceiverAddress(), addbaRequest->getTid());
    blockAckAgreements[agreementId] = blockAckAgreement;
}

void OriginatorBlockAckAgreementHandler::processTransmittedDelbaRequest(Ieee80211Delba *delbaRequest)
{
    Tid tid = delbaRequest->getTid();
    MACAddress receiverAddr = delbaRequest->getReceiverAddress();
    auto agreementId = std::make_pair(receiverAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    if (it != blockAckAgreements.end())
        blockAckAgreements.erase(it);
    else
        throw cRuntimeError("Block Ack Agreement does not exist");
}

void OriginatorBlockAckAgreementHandler::processReceivedAddbaResponse(Ieee80211AddbaResponse *addbaResponse)
{
    Tid tid = addbaResponse->getTid();
    MACAddress transmitterAddr = addbaResponse->getTransmitterAddress();
    auto agreementId = std::make_pair(transmitterAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    if (it != blockAckAgreements.end()) {
        OriginatorBlockAckAgreement *agreement = it->second;
        int bufferSize = addbaResponse->getBufferSize();
        simtime_t blockAckTimeoutValue = addbaResponse->getBlockAckTimeoutValue();
        // TODO: policy: when do we accept the new values?
        agreement->setIsAddbaResponseReceived(true);
        agreement->setBufferSize(bufferSize);
        agreement->setBlockAckTimeoutValue(blockAckTimeoutValue);
    }
}

simtime_t OriginatorBlockAckAgreementHandler::getAddbaRequestDuration(Ieee80211AddbaRequest *addbaReq) const
{
    const IIeee80211Mode *mode = rateSelection->getModeForUnicastDataOrMgmtFrame(addbaReq);
    return mode->getDuration(addbaReq->getBitLength());
}

simtime_t OriginatorBlockAckAgreementHandler::getAddbaRequestEarlyTimeout() const
{
    return sifs + slotTime + phyRxStartDelay;
}

Ieee80211AddbaRequest* OriginatorBlockAckAgreementHandler::buildAddbaRequest(MACAddress receiverAddr, Tid tid, int startingSequenceNumber)
{
    Ieee80211AddbaRequest *addbaRequest = new Ieee80211AddbaRequest("AddbaReq");
    addbaRequest->setReceiverAddress(receiverAddr);
    addbaRequest->setTid(tid);
    addbaRequest->setAMsduSupported(isAMsduSupported);
    addbaRequest->setBlockAckTimeoutValue(blockAckTimeoutValue);
    addbaRequest->setBufferSize(maximumAllowedBufferSize);
    // The Block Ack Policy subfield is set to 1 for immediate Block Ack and 0 for delayed Block Ack.
    addbaRequest->setBlockAckPolicy(isDelayedBlockAckPolicySupported ? 0 : 1);
    addbaRequest->setStartingSequenceNumber(startingSequenceNumber);
    setFrameMode(addbaRequest, rateSelection->getModeForUnicastDataOrMgmtFrame(addbaRequest));
    // FIXME: Within all management frames sent by the QoS STA, the Duration field contains a duration
    // value as defined in 8.2.5.
    addbaRequest->setDuration(rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK) + sifs);
    return addbaRequest;
}

void OriginatorBlockAckAgreementHandler::processReceivedDelba(Ieee80211Delba* delba)
{
    Tid tid = delba->getTid();
    MACAddress transmitterAddr = delba->getTransmitterAddress();
    auto agreementId = std::make_pair(transmitterAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    if (it != blockAckAgreements.end()) {
        delete it->second;
        blockAckAgreements.erase(it);
    }
    else {
        EV_DETAIL << "Received Delba frame but agreement does not exist" << endl;
    }
}

OriginatorBlockAckAgreement* OriginatorBlockAckAgreementHandler::getAgreement(MACAddress receiverAddr, Tid tid)
{
    auto agreementId = std::make_pair(receiverAddr, tid);
    auto it = blockAckAgreements.find(agreementId);
    return it != blockAckAgreements.end() ? it->second : nullptr;
}

Ieee80211Delba* OriginatorBlockAckAgreementHandler::buildDelba(MACAddress receiverAddr, Tid tid, int reasonCode)
{
    Ieee80211Delba *delba = new Ieee80211Delba();
    delba->setReceiverAddress(receiverAddr);
    delba->setTid(tid);
    delba->setReasonCode(reasonCode);
    // The Initiator subfield indicates if the originator or the recipient of the data is sending this frame.
    delba->setInitiator(true);
    delba->setDuration(rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK) + sifs);
    return delba;
}

// TODO: move this part to somewhere else
Ieee80211Frame *OriginatorBlockAckAgreementHandler::setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const
{
    ASSERT(frame->getControlInfo() == nullptr);
    Ieee80211TransmissionRequest *ctrl = new Ieee80211TransmissionRequest();
    ctrl->setMode(mode);
    frame->setControlInfo(ctrl);
    return frame;
}

} // namespace ieee80211
} // namespace inet

