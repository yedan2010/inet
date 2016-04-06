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

#ifndef __INET_CTSPROCEDURE_H
#define __INET_CTSPROCEDURE_H

#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/contract/IRx.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"


namespace inet {
namespace ieee80211 {

//
// 9.3.2.6 CTS procedure
//
// A STA that is addressed by an RTS frame shall transmit a CTS frame after a SIFS period if the NAV at the
// STA receiving the RTS frame indicates that the medium is idle. If the NAV at the STA receiving the RTS
// indicates the medium is not idle, that STA shall not respond to the RTS frame. The RA field of the CTS frame
// shall be the value obtained from the TA field of the RTS frame to which this CTS frame is a response. The
// Duration field in the CTS frame shall be the duration field from the received RTS frame, adjusted by
// subtraction of aSIFSTime and the number of microseconds required to transmit the CTS frame at a data rate
// determined by the rules in 9.7.
//
class INET_API CtsProcedure
{
    protected:
        IRx *rx = nullptr;
        IRateSelection *rateSelection = nullptr;

        simtime_t sifs = -1;
        simtime_t slotTime = -1;
        simtime_t phyRxStartDelay = -1;

    protected:
        Ieee80211Frame *setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const;
        Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const;
        Ieee80211RTSFrame *buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const;

    public:
        CtsProcedure(IRx *rx, IRateSelection *rateSelection);

        Ieee80211CTSFrame * buildCts(Ieee80211RTSFrame *rtsFrame);
        Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const;

        void processReceivedRts(Ieee80211RTSFrame *rtsFrame);
        void processTransmittedCts(Ieee80211CTSFrame *rtsFrame);

        simtime_t getCtsDuration() const;
        simtime_t getCtsEarlyTimeout() const;
        simtime_t getCtsFullTimeout() const;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_CTSPROCEDURE_H
