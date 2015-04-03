//
// Copyright (C) 2015 OpenSim Ltd.
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_IEEE80211MACAUTORATE_H
#define __INET_IEEE80211MACAUTORATE_H

#include "inet/common/INETDefs.h"
#include "inet/common/INETMath.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/physicallayer/ieee80211/mode/IIeee80211Mode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"

namespace inet {
namespace ieee80211 {

using namespace physicallayer;

class INET_API Ieee80211MacAutoRate
{
    public:
        enum RateControlMode {
            RATE_ARF,    // Auto Rate Fallback
            RATE_AARF,    // Adaptatice ARF
            RATE_CR,    // Constant Rate
        };

    public:
        RateControlMode rateControlMode = (RateControlMode)-1;

        // Variables used by the auto bit rate
        bool forceBitRate = false;    //if true the
        unsigned int intrateIndex = 0;
        int contI = 0;
        int contJ = 0;
        int samplingCoeff = 50; // XXX value comes from the Ieee80211MAC::initialize()
        double recvdThroughput = 0; // XXX value comes from the Ieee80211MAC::initialize()
        int autoBitrate = 0;
        int rateIndex = 0;
        int successCounter = 0;
        int failedCounter = 0;
        bool recovery = false;
        int timer = 0;
        int successThreshold = 0;
        int maxSuccessThreshold = 0;
        int timerTimeout = 0;
        int minSuccessThreshold = 0;
        int minTimerTimeout = 0;
        double successCoeff = NaN;
        double timerCoeff = NaN;
        double _snr = NaN;
        double snr = NaN;
        double lossRate = NaN;
        simtime_t timeStampLastMessageReceived = SIMTIME_ZERO; // XXX value comes from the Ieee80211MAC::initialize()

    public:
        Ieee80211MacAutoRate(bool forceBitRate, int minSuccessThreshold, int minTimerTimeout, int timerTimeout, int successThreshold, int autoBitrate, double successCoeff, double timerCoeff, int maxSuccessThreshold);

        RateControlMode getRateControlMode() const { return rateControlMode; }
        bool isForceBitRate() const { return forceBitRate; }
        void increaseReceivedThroughput(unsigned int bitLength);
        const simtime_t getTimeStampLastMessageReceived() const { return timeStampLastMessageReceived; }

        void setTimerTimeout(int timerTimout);
        void setSuccessThreshold(int successThreshold);
        void setLastMessageTimeStamp();
        void someKindOfFunction1(const Ieee80211ReceptionIndication *controlInfo);
        void someKindOfFunction2();
        void reportFailure();
        const IIeee80211Mode *computeFasterDataFrameMode(const Ieee80211ModeSet *modeSet, const IIeee80211Mode *dataFrameMode);
        const IIeee80211Mode *computeSlowerDataFrameMode(const Ieee80211ModeSet *modeSet, const IIeee80211Mode *dataFrameMode, unsigned int retryCounter, bool needNormalFeedback);
        void reportRecoveryFailure();

    };
} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_IEEE80211MACAUTORATE_H
