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

#ifndef __INET_ORIGINATORBLOCKACKPROCEDURE_H
#define __INET_ORIGINATORBLOCKACKPROCEDURE_H

#include "inet/linklayer/ieee80211/mac/common/Ieee80211Defs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"

namespace inet {
namespace ieee80211 {

class INET_API OriginatorBlockAckProcedure
{
    protected:
        IRateSelection *rateSelection = nullptr;

        simtime_t sifs = -1;
        simtime_t slotTime = -1;
        simtime_t phyRxStartDelay = 1;

    public:
        OriginatorBlockAckProcedure(IRateSelection *rateSelection);

        virtual Ieee80211BlockAckReq *buildCompressedBlockAckReqFrame(const MACAddress& receiverAddress, Tid tid, int startingSequenceNumber) const;
        virtual Ieee80211BlockAckReq *buildBasicBlockAckReqFrame(const MACAddress& receiverAddress, Tid tid, int startingSequenceNumber) const;

        simtime_t getBlockAckEarlyTimeout() const;
        simtime_t getBlockAckFullTimeout(Ieee80211BlockAckReq* blockAckReq) const;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_ORIGINATORBLOCKACKPROCEDURE_H
