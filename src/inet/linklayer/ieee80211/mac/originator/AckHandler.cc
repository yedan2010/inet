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

AckHandler::Status& AckHandler::getAckStatus(SequenceControlField id)
{
    auto it = ackStatuses.find(id);
    if (it == ackStatuses.end()) {
        ackStatuses[id] = Status::FRAME_NOT_YET_TRANSMITTED;
        return ackStatuses[id];
    }
    return it->second;
}

AckHandler::Status AckHandler::getAckStatus(Ieee80211DataOrMgmtFrame* frame)
{
    auto id = SequenceControlField(frame->getSequenceNumber(),frame->getFragmentNumber());
    auto it = ackStatuses.find(id);
    if (it == ackStatuses.end()) {
        ackStatuses[id] = Status::FRAME_NOT_YET_TRANSMITTED;
        return Status::FRAME_NOT_YET_TRANSMITTED;
    }
    return it->second;
}

void AckHandler::processReceivedAck(Ieee80211ACKFrame* ack, Ieee80211DataOrMgmtFrame *ackedFrame)
{
    auto id = SequenceControlField(ackedFrame->getSequenceNumber(),ackedFrame->getFragmentNumber());
    Status &status = getAckStatus(id);
    if (status == Status::FRAME_NOT_YET_TRANSMITTED)
        throw cRuntimeError("ackedFrame = %s is not yet transmitted", ackedFrame->getName());
    status = Status::NORMAL_ACK_ARRIVED;
}

std::set<SequenceControlField> AckHandler::processReceivedBlockAck(Ieee80211BlockAck* blockAck)
{
    printAckStatuses();
    std::set<SequenceControlField> ackedFrames;
    // Table 8-16—BlockAckReq frame variant encoding
    if (auto basicBlockAck = dynamic_cast<Ieee80211BasicBlockAck *>(blockAck)) {
        int startingSeqNum = basicBlockAck->getStartingSequenceNumber();
        for (int seqNum = 0; seqNum < 64; seqNum++) {
            BitVector bitmap = basicBlockAck->getBlockAckBitmap(seqNum);
            for (int fragNum = 0; fragNum < 16; fragNum++) { // TODO: declare these const values
                auto id = SequenceControlField(startingSeqNum + seqNum, fragNum);
                Status& status = getAckStatus(id);
                if (status == Status::WAITING_FOR_BLOCK_ACK) {
                    bool acked = bitmap.getBit(fragNum) == 1;
                    if (acked) ackedFrames.insert(id);
                    status = acked ? Status::BLOCK_ACK_ARRIVED_ACKED : Status::BLOCK_ACK_ARRIVED_UNACKED;
                }
                else ; // TODO: erroneous BlockAck
            }
        }
    }
    else if (auto compressedBlockAck = dynamic_cast<Ieee80211CompressedBlockAck *>(blockAck)) {
        int startingSeqNum = compressedBlockAck->getStartingSequenceNumber();
        BitVector bitmap = compressedBlockAck->getBlockAckBitmap();
        for (int seqNum = 0; seqNum < 64; seqNum++) {
            auto id = SequenceControlField(startingSeqNum + seqNum, 0);
            Status& status = getAckStatus(id);
            if (status == Status::WAITING_FOR_BLOCK_ACK) {
                bool acked = bitmap.getBit(seqNum) == 1;
                status = acked ? Status::BLOCK_ACK_ARRIVED_ACKED : Status::BLOCK_ACK_ARRIVED_UNACKED;
                if (acked) ackedFrames.insert(id);
            }
            else ; // TODO: erroneous BlockAck
        }
    }
    else {
        throw cRuntimeError("Multi-TID BlockReq is unimplemented");
    }
    printAckStatuses();
    return ackedFrames;
}

void AckHandler::processTransmittedNonQoSFrame(Ieee80211DataOrMgmtFrame* frame)
{
    auto id = SequenceControlField(frame->getSequenceNumber(),frame->getFragmentNumber());
    ackStatuses[id] = Status::WAITING_FOR_NORMAL_ACK;
}

void AckHandler::processTransmittedMgmtFrame(Ieee80211ManagementFrame* frame)
{
    auto id = SequenceControlField(frame->getSequenceNumber(),frame->getFragmentNumber());
    ackStatuses[id] = Status::WAITING_FOR_NORMAL_ACK;
}

void AckHandler::processTransmittedQoSData(Ieee80211DataFrame* frame)
{
    ASSERT(frame->getType() == ST_DATA_WITH_QOS);
    auto id = SequenceControlField(frame->getSequenceNumber(),frame->getFragmentNumber());
    switch (frame->getAckPolicy()) {
        case NORMAL_ACK : ackStatuses[id] = Status::WAITING_FOR_NORMAL_ACK; break;
        case BLOCK_ACK : ackStatuses[id] = Status::BLOCK_ACK_NOT_YET_REQUESTED; break;
        case NO_ACK : ackStatuses[id] = Status::NO_ACK_REQUIRED; break;
        case NO_EXPLICIT_ACK : throw cRuntimeError("Unimplemented"); /* TODO: ACKED by default? */ break;
        default: throw cRuntimeError("Unknown Ack Policy = %d", frame->getAckPolicy());
    }
}

void AckHandler::processTransmittedBlockAckReq(Ieee80211BlockAckReq* blockAckReq)
{
    printAckStatuses();
    for (auto &ackStatus : ackStatuses) {
        auto seqCtrlField = ackStatus.first;
        auto &status = ackStatus.second;
        if (auto basicBlockAckReq = dynamic_cast<Ieee80211BasicBlockAckReq *>(blockAckReq)) {
            int startingSeqNum = basicBlockAckReq->getStartingSequenceNumber();
            if (status == Status::BLOCK_ACK_NOT_YET_REQUESTED)
                if (seqCtrlField.getSequenceNumber() >= startingSeqNum)
                    status = Status::WAITING_FOR_BLOCK_ACK;
        }
        else if (auto compressedBlockAckReq = dynamic_cast<Ieee80211CompressedBlockAckReq *>(blockAckReq)) {
            int startingSeqNum = compressedBlockAckReq->getStartingSequenceNumber();
            if (status == Status::BLOCK_ACK_NOT_YET_REQUESTED)
                if (seqCtrlField.getSequenceNumber() >= startingSeqNum && seqCtrlField.getFragmentNumber() == 0) // TODO: ASSERT(seqCtrlField.second == 0)?
                    status = Status::WAITING_FOR_BLOCK_ACK;
        }
        else
            throw cRuntimeError("Multi-TID BlockReq is unimplemented");
    }
    printAckStatuses();
}

void AckHandler::frameGotInProgress(Ieee80211DataOrMgmtFrame* dataOrMgmtFrame)
{
    auto id = SequenceControlField(dataOrMgmtFrame->getSequenceNumber(), dataOrMgmtFrame->getFragmentNumber());
    ackStatuses[id] = Status::FRAME_NOT_YET_TRANSMITTED;
}

int AckHandler::getNumberOfFramesWithStatus(Status status)
{
    int count = 0;
    for (auto ackStatus : ackStatuses)
        if (ackStatus.second == status)
            count++;
    return count;
}


std::string AckHandler::printStatus(Status status)
{
    switch (status) {
        case Status::FRAME_NOT_YET_TRANSMITTED : return "FRAME_NOT_YET_TRANSMITTED";
        case Status::NO_ACK_REQUIRED : return "NO_ACK_REQUIRED";
        case Status::BLOCK_ACK_NOT_YET_REQUESTED : return "BLOCK_ACK_NOT_YET_REQUESTED";
        case Status::WAITING_FOR_NORMAL_ACK : return "WAITING_FOR_NORMAL_ACK";
        case Status::NORMAL_ACK_NOT_ARRIVED : return "NORMAL_ACK_NOT_ARRIVED";
        case Status::BLOCK_ACK_ARRIVED_UNACKED : return "BLOCK_ACK_ARRIVED_UNACKED";
        case Status::BLOCK_ACK_ARRIVED_ACKED  : return "BLOCK_ACK_ARRIVED_ACKED";
        case Status::WAITING_FOR_BLOCK_ACK  : return "WAITING_FOR_BLOCK_ACK";
        case Status::NORMAL_ACK_ARRIVED  : return "NORMAL_ACK_ARRIVED";
        default: throw cRuntimeError("Unknown status");
    }
}

void AckHandler::printAckStatuses()
{
    for (auto ackStatus : ackStatuses) {
        std::cout << "Seq Num = " << ackStatus.first.getSequenceNumber() << " " << "Frag Num = " << (int)ackStatus.first.getFragmentNumber() << std::endl;
        std::cout << "Status = " << printStatus(ackStatus.second) << std::endl;
    }
    std::cout << "=========================================" << std::endl;
}

} /* namespace ieee80211 */
} /* namespace inet */
