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

#include "OriginatorBlockAckProcedure.h"

namespace inet {
namespace ieee80211 {

OriginatorBlockAckProcedure::OriginatorBlockAckProcedure(IRateSelection* rateSelection) :
    rateSelection(rateSelection)
{
    this->sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
    this->slotTime = rateSelection->getSlowestMandatoryMode()->getSlotTime();
    this->phyRxStartDelay = rateSelection->getSlowestMandatoryMode()->getPhyRxStartDelay();
}

Ieee80211BlockAckReq* OriginatorBlockAckProcedure::buildCompressedBlockAckReqFrame(const MACAddress& receiverAddress, Tid tid, int startingSequenceNumber) const
{
    throw cRuntimeError("Unsupported feature");
    Ieee80211CompressedBlockAckReq *blockAckReq = new Ieee80211CompressedBlockAckReq("CompressedBlockAckReq");
    //TODO: blockAckReq->setTransmitterAddress(params->getAddress());
    blockAckReq->setReceiverAddress(receiverAddress);
    blockAckReq->setDuration(sifs);
    blockAckReq->setStartingSequenceNumber(startingSequenceNumber);
    blockAckReq->setTidInfo(tid);
    return blockAckReq;
}

Ieee80211BlockAckReq* OriginatorBlockAckProcedure::buildBasicBlockAckReqFrame(const MACAddress& receiverAddress, Tid tid, int startingSequenceNumber) const
{
    Ieee80211BasicBlockAckReq *blockAckReq = new Ieee80211BasicBlockAckReq("BasicBlockAckReq");
    blockAckReq->setReceiverAddress(receiverAddress);
    // For a BlockAckReq frame, the Duration/ID field is set to the estimated time required to
    // transmit one ACK or BlockAck frame, as applicable, plus one SIFS interval.
    blockAckReq->setStartingSequenceNumber(startingSequenceNumber);
    blockAckReq->setTidInfo(tid);
    auto blockAckDur = rateSelection->getModeForControlFrame(blockAckReq)->getDuration(LENGTH_BASIC_BLOCKACK);
    blockAckReq->setDuration(blockAckDur + sifs);
    return blockAckReq;
}


simtime_t OriginatorBlockAckProcedure::getBlockAckEarlyTimeout() const
{
    return sifs + slotTime + phyRxStartDelay; // see getAckEarlyTimeout()
}

simtime_t OriginatorBlockAckProcedure::getBlockAckFullTimeout(Ieee80211BlockAckReq* blockAckReq) const
{
    if (dynamic_cast<Ieee80211BasicBlockAckReq*>(blockAckReq)) {
        return sifs + slotTime + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_BASIC_BLOCKACK);
    }
    else
        throw cRuntimeError("Unsupported BlockAck Request");
}


} /* namespace ieee80211 */
} /* namespace inet */

