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

#ifndef __INET_ACKHANDLER_H
#define __INET_ACKHANDLER_H

#include "inet/linklayer/ieee80211/mac/common/SequenceControlField.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include <map>

namespace inet {
namespace ieee80211 {

class INET_API AckHandler
{
    public:
        enum class Status {
            FRAME_NOT_YET_TRANSMITTED,
            NO_ACK_REQUIRED,
            BLOCK_ACK_NOT_YET_REQUESTED,
            WAITING_FOR_NORMAL_ACK,
            WAITING_FOR_BLOCK_ACK,
            NORMAL_ACK_NOT_ARRIVED,
            NORMAL_ACK_ARRIVED,
            BLOCK_ACK_ARRIVED_UNACKED,
            BLOCK_ACK_ARRIVED_ACKED
        };
    protected:
        std::map<SequenceControlField, Status> ackStatuses;

    protected:
        virtual Status& getAckStatus(SequenceControlField id);
        std::string printStatus(Status status);
        void printAckStatuses();

    public:
        virtual void processReceivedAck(Ieee80211ACKFrame *ack, Ieee80211DataOrMgmtFrame *ackedFrame);
        virtual std::set<SequenceControlField> processReceivedBlockAck(Ieee80211BlockAck *blockAck);

        virtual void frameGotInProgress(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame);
        virtual void processTransmittedMgmtFrame(Ieee80211ManagementFrame *frame);
        virtual void processTransmittedQoSData(Ieee80211DataFrame *frame);
        virtual void processTransmittedNonQoSFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual void processTransmittedBlockAckReq(Ieee80211BlockAckReq *blockAckReq);

        virtual Status getAckStatus(Ieee80211DataOrMgmtFrame *frame);
        virtual int getNumberOfFramesWithStatus(Status status);
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_ACKHANDLER_H
