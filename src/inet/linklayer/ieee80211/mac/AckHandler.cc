//
// Copyright (C) 2015 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
// 

#include "AckHandler.h"

namespace inet {
namespace ieee80211 {

AckHandler::Status& AckHandler::getAckStatus(std::pair<uint16_t, uint8_t> id)
{
    auto it = ackInfo.find(id);
    if (it == ackInfo.end()) {
        ackInfo[id] = Status::UNKNOWN;
        return ackInfo[id];
    }
    return it->second;
}

AckHandler::Status AckHandler::getAckStatus(Ieee80211DataOrMgmtFrame* frame)
{
    auto id = std::make_pair(frame->getSequenceNumber(),frame->getFragmentNumber());
    auto it = ackInfo.find(id);
    if (it == ackInfo.end()) {
        ackInfo[id] = Status::UNKNOWN;
        return Status::UNKNOWN;
    }
    return it->second;
}

void AckHandler::invalidateAckStatus(Ieee80211DataOrMgmtFrame* frame)
{
    auto id = std::make_pair(frame->getSequenceNumber(),frame->getFragmentNumber());
    ackInfo[id] = Status::UNKNOWN;
}

void AckHandler::invalidateAckStatuses(std::set<std::pair<uint16_t, uint8_t> > seqAndFragNums)
{
    for (auto id : seqAndFragNums)
        ackInfo[id] = Status::UNKNOWN;
}

void AckHandler::processReceivedAck(Ieee80211ACKFrame* ack, Ieee80211DataOrMgmtFrame *ackedFrame)
{
    auto id = std::make_pair(ackedFrame->getSequenceNumber(),ackedFrame->getFragmentNumber());
    Status status = getAckStatus(id);
    if (status == Status::UNKNOWN)
        throw cRuntimeError("ackedFrame = %s is not yet transmitted", ackedFrame->getName());
    status = Status::ARRIVED_ACKED;
}

std::set<std::pair<uint16_t, uint8_t>> AckHandler::processReceivedBlockAck(Ieee80211BlockAck* blockAck)
{
    std::set<std::pair<uint16_t, uint8_t>> ackedFrames;
    // Table 8-16—BlockAckReq frame variant encoding
    if (auto basicBlockAck = check_and_cast<Ieee80211BasicBlockAck *>(blockAck)) {
        int startingSeqNum = basicBlockAck->getStartingSequenceNumber();
        for (int seqNum = 0; seqNum < 64; seqNum++) {
            BitVector bitmap = basicBlockAck->getBlockAckBitmap(seqNum);
            for (int fragNum = 0; fragNum < 16; fragNum++) { // TODO: declare these const values
                auto id = std::make_pair(startingSeqNum + seqNum, fragNum);
                Status& status = getAckStatus(id);
                if (status != Status::NOT_YET_REQUESTED && status != Status::UNKNOWN && status != Status::NO_ACK_REQUIRED) {
                    bool acked = bitmap.getBit(fragNum) == 1;
                    if (acked) ackedFrames.insert(id);
                    Status newStatus = acked ? Status::ARRIVED_ACKED : Status::ARRIVED_UNACKED;
                    if (status != Status::WAITING_FOR_RESPONSE) ASSERT(status == newStatus);
                    status = newStatus;
                }
                else ; // TODO: erroneous BlockAck
            }
        }
    }
    else if (auto compressedBlockAck = check_and_cast<Ieee80211CompressedBlockAck *>(blockAck)) {
        int startingSeqNum = compressedBlockAck->getStartingSequenceNumber();
        BitVector bitmap = compressedBlockAck->getBlockAckBitmap();
        for (int seqNum = 0; seqNum < 64; seqNum++) {
            auto id = std::make_pair(startingSeqNum + seqNum, 0);
            Status& status = getAckStatus(id);
            if (status != Status::NOT_YET_REQUESTED && status != Status::UNKNOWN && status != Status::NO_ACK_REQUIRED) {
                bool acked = bitmap.getBit(seqNum) == 1;
                if (acked) ackedFrames.insert(id);
                Status newStatus = acked ? Status::ARRIVED_ACKED : Status::ARRIVED_UNACKED;
                if (status != Status::WAITING_FOR_RESPONSE) ASSERT(status == newStatus);
                status = newStatus;
            }
            else ; // TODO: erroneous BlockAck
        }
    }
    else {
        throw cRuntimeError("Multi-TID BlockReq is unimplemented");
    }
    return ackedFrames;
}

void AckHandler::processTransmittedNonQoSFrame(Ieee80211DataOrMgmtFrame* frame)
{
    auto id = std::make_pair(frame->getSequenceNumber(),frame->getFragmentNumber());
    ackInfo[id] = Status::WAITING_FOR_RESPONSE;
}

void AckHandler::processTransmittedQoSData(Ieee80211DataFrame* frame)
{
    ASSERT(frame->getType() == ST_DATA_WITH_QOS);
    auto id = std::make_pair(frame->getSequenceNumber(),frame->getFragmentNumber());
    switch (frame->getAckPolicy()) {
        case NORMAL_ACK : ackInfo[id] = Status::WAITING_FOR_RESPONSE; break;
        case BLOCK_ACK : ackInfo[id] = Status::NOT_YET_REQUESTED; break;
        case NO_ACK : ackInfo[id] = Status::NO_ACK_REQUIRED; break;
        case NO_EXPLICIT_ACK : throw cRuntimeError("Unimplemented"); /* TODO: ACKED by default? */ break;
        default: throw cRuntimeError("Unknown Ack Policy = %d", frame->getAckPolicy());
    }
}

void AckHandler::processTransmittedBlockAckReq(Ieee80211BlockAckReq* blockAckReq)
{
    for (auto info : ackInfo) {
        auto seqCtrlField = info.first;
        auto &status = info.second;
        if (auto basicBlockAckReq = dynamic_cast<Ieee80211BasicBlockAckReq *>(blockAckReq)) {
            int startingSeqNum = basicBlockAckReq->getStartingSequenceNumber();
            if (status != Status::UNKNOWN && status != Status::NO_ACK_REQUIRED)
                if (seqCtrlField.first >= startingSeqNum)
                    status = Status::WAITING_FOR_RESPONSE;
        }
        else if (auto compressedBlockAckReq = dynamic_cast<Ieee80211CompressedBlockAckReq *>(blockAckReq)) {
            int startingSeqNum = compressedBlockAckReq->getStartingSequenceNumber();
            if (status != Status::UNKNOWN && status != Status::NO_ACK_REQUIRED)
                if (seqCtrlField.first >= startingSeqNum && seqCtrlField.second == 0) // TODO: ASSERT(seqCtrlField.second == 0)?
                    status = Status::WAITING_FOR_RESPONSE;
        }
        else
            throw cRuntimeError("Multi-TID BlockReq is unimplemented");
    }
}


} /* namespace ieee80211 */
} /* namespace inet */
