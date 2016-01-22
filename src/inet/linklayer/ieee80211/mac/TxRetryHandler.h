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

#ifndef __INET_TXRETRYANDLIFETIMEHANDLER_H
#define __INET_TXRETRYANDLIFETIMEHANDLER_H

#include "AccessCategory.h"
#include "IMacParameters.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "TimingParameters.h"

namespace inet {
namespace ieee80211 {

//
// References: 9.19.2.6 Retransmit procedures (IEEE 802.11-2012 STD)
//             802.11 Reference Design: Recovery Procedures and Retransmit Limits
//             (https://warpproject.org/trac/wiki/802.11/MAC/Lower/Retransmissions)
//
class INET_API TxRetryHandler {

    protected:
        IMacParameters *params = nullptr;
        TimingParameters *timingParameters = nullptr;
        // TODO: use seqNum, fragNum
        std::map<long int, int> shortRetryCounter; // SRC
        std::map<long int, int> longRetryCounter; // LRC
        // TODO: add lifetime

        int stationLongRetryCounter = 0; // SLRC
        int stationShortRetryCounter = 0; // SSRC
        int cw = 0; // Contention window

    protected:
        void incrementCounter(Ieee80211Frame* frame, std::map<long int, int>& retryCounter);
        void incrementStationSrc();
        void incrementStationLrc();
        void resetStationSrc() { stationShortRetryCounter = 0; }
        void resetStationLrc() { stationLongRetryCounter = 0; }
        void resetContentionWindow() { cw = timingParameters->getCwMin(); }
        int doubleCw(int cw);
        int getRc(Ieee80211Frame* frame, std::map<long int, int>& retryCounter);
        bool isMulticastFrame(Ieee80211Frame *frame);

    public:
        // The contention window (CW) parameter shall take an initial value of aCWmin.
        TxRetryHandler(IMacParameters *params, TimingParameters *timingParameters) :
                params(params), timingParameters(timingParameters), cw(timingParameters->getCwMin()) { }

        void multicastFrameTransmitted();

        virtual void ctsFrameReceived();
        virtual void ackFrameReceived(Ieee80211DataOrMgmtFrame *ackedFrame);
        virtual void blockAckFrameReceived();

        void rtsFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *protectedFrame);
        void dataOrMgmtFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *failedFrame);

        bool isDataOrMgtmFrameRetryLimitReached(Ieee80211DataOrMgmtFrame* failedFrame);
        bool isRtsFrameRetryLimitReached(Ieee80211DataOrMgmtFrame* protectedFrame);
        bool isLifetimeExpired(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame); // TODO: remove

        int getCw() { return cw; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_TXRETRYANDLIFETIMEHANDLER_H
