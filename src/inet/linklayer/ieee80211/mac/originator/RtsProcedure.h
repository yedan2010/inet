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

#ifndef __INET_RTSPROCEDURE_H
#define __INET_RTSPROCEDURE_H

#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

class INET_API RtsProcedure
{
    protected:
        IRateSelection *rateSelection = nullptr;

        simtime_t sifs = -1;
        simtime_t slotTime = -1;
        simtime_t phyRxStartDelay = -1;

        int rtsThreshold = -1;

    protected:
        Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const;
        Ieee80211RTSFrame *buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const;
        bool isBroadcastOrMulticast(Ieee80211Frame *frame) const;
        const IIeee80211Mode *getFrameMode(Ieee80211Frame *frame) const;
        Ieee80211Frame *setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const;

    public:
        RtsProcedure(IRateSelection *rateSelection);

        virtual Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const;

        virtual int getRtsThreshold() { return rtsThreshold; }

        simtime_t getCtsDuration() const;
        simtime_t getCtsEarlyTimeout() const;
        simtime_t getCtsFullTimeout() const;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_RTSPROCEDURE_H
