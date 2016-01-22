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

#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include <map>

namespace inet {
namespace ieee80211 {

class INET_API AckHandler
{
    public:
        enum class Status {
            ARRIVED_ACKED,
            ARRIVED_UNACKED,
            NO_ACK_REQUIRED,
            NOT_ARRIVED_IN_TIME,
            NOT_YET_REQUESTED,
            UNKNOWN,
            WAITING_FOR_RESPONSE
        };
    protected:
        std::map<std::pair<uint16_t, uint8_t>, Status> ackInfo;

    protected:
        virtual Status& getAckStatus(std::pair<uint16_t, uint8_t> id);

    public:
        virtual void processReceivedAck(Ieee80211ACKFrame *ack, Ieee80211DataOrMgmtFrame *ackedFrame);
        virtual std::set<std::pair<uint16_t, uint8_t>> processReceivedBlockAck(Ieee80211BlockAck *blockAck);

        virtual void processTransmittedQoSData(Ieee80211DataFrame *frame);
        virtual void processTransmittedNonQoSFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual void processTransmittedBlockAckReq(Ieee80211BlockAckReq *blockAckReq);

        virtual void invalidateAckStatus(Ieee80211DataOrMgmtFrame *frame);
        virtual void invalidateAckStatuses(std::set<std::pair<uint16_t, uint8_t>> seqAndFragNums);

        virtual Status getAckStatus(Ieee80211DataOrMgmtFrame *frame);
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_ACKHANDLER_H
