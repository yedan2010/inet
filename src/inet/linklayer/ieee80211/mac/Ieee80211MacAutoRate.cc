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

#include "inet/linklayer/ieee80211/mac/Ieee80211MacAutoRate.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"

namespace inet {
namespace ieee80211 {

Ieee80211MacAutoRate::Ieee80211MacAutoRate(Ieee80211Mac *ieee80211mac, bool forceBitRate, int minSuccessThreshold, int minTimerTimeout, int timerTimeout, int successThreshold, int autoBitrate, double successCoeff, double timerCoeff, int maxSuccessThreshold) :
        ieee80211mac(ieee80211mac),
        forceBitRate(forceBitRate),
        autoBitrate(autoBitrate),
        successThreshold(successThreshold),
        timerTimeout(timerTimeout),
        minSuccessThreshold(minSuccessThreshold),
        minTimerTimeout(minTimerTimeout)
{
    switch (autoBitrate) {
        case 0:
            rateControlMode = RATE_CR;
            EV_DEBUG << "MAC Transmission algorithm : Constant Rate" << endl;
            break;
        case 1:
            rateControlMode = RATE_ARF;
            EV_DEBUG << "MAC Transmission algorithm : ARF Rate" << endl;
            break;
        case 2:
            rateControlMode = RATE_AARF;
            this->successCoeff = successCoeff;
            this->timerCoeff = timerCoeff;
            this->maxSuccessThreshold = maxSuccessThreshold;
            EV_DEBUG << "MAC Transmission algorithm : AARF Rate" << endl;
            break;
        default:
            throw cRuntimeError("Invalid autoBitrate parameter: '%d'", autoBitrate);
            break;
    }
}

void Ieee80211MacAutoRate::increaseReceivedThroughput(unsigned int bitLength)
{
    recvdThroughput += ((bitLength / (simTime() - timestampLastMessageReceived)) / 1000000) / samplingCoeff;
}

void Ieee80211MacAutoRate::setLastMessageTimestamp()
{
    timestampLastMessageReceived = simTime();
}

void Ieee80211MacAutoRate::someKindOfFunction1(const Ieee80211Frame* frame)
{
    const Ieee80211ReceptionIndication *controlInfo = dynamic_cast<const Ieee80211ReceptionIndication *>(frame->getControlInfo());
    if (controlInfo) {
        if (contJ % 10 == 0) {
            snr = _snr;
            contJ = 0;
            _snr = 0;
        }
        contJ++;
        _snr += controlInfo->getSnr() / 10;
        lossRate = controlInfo->getLossRate();
        delete controlInfo;
    }
    if (contI % samplingCoeff == 0) {
        contI = 0;
        recvdThroughput = 0;
    }
    contI++;
    if (getTimestampLastMessageReceived() == SIMTIME_ZERO)
        setLastMessageTimestamp();
    else {
        if (frame)
            increaseReceivedThroughput(frame->getBitLength());
        setLastMessageTimestamp();
    }
}

void Ieee80211MacAutoRate::reportRecoveryFailure()
{
    if (rateControlMode == RATE_AARF) {
        setSuccessThreshold((int)(std::min((double)successThreshold * successCoeff, (double)maxSuccessThreshold)));
        setTimerTimeout((int)(std::max((double)minTimerTimeout, (double)(successThreshold * timerCoeff))));
    }
}

void Ieee80211MacAutoRate::setSuccessThreshold(int successThreshold)
{
    if (successThreshold >= minSuccessThreshold)
        this->successThreshold = successThreshold;
    else
        throw cRuntimeError("successThreshold is less than minSuccessThreshold");
}

void Ieee80211MacAutoRate::setTimerTimeout(int timerTimout)
{
    if (timerTimout >= minTimerTimeout)
        this->timerTimeout = timerTimout;
    else
        throw cRuntimeError("timerTimout is less than minTimerTimeout");
}

const IIeee80211Mode* Ieee80211MacAutoRate::computeFasterDataFrameMode(const Ieee80211ModeSet* modeSet, const IIeee80211Mode* dataFrameMode)
{
    if (rateControlMode == RATE_CR)
        return nullptr;
    successCounter++;
    failedCounter = 0;
    recovery = false;
    if ((successCounter == successThreshold || timer == timerTimeout) && modeSet->getFasterMode(dataFrameMode))
    {
        timer = 0;
        successCounter = 0;
        recovery = true;
        return modeSet->getFasterMode(dataFrameMode);
    }
    return nullptr;
}

const IIeee80211Mode* Ieee80211MacAutoRate::computeSlowerDataFrameMode(const Ieee80211ModeSet *modeSet, const IIeee80211Mode *dataFrameMode, unsigned int retryCounter, bool needNormalFeedback)
{
    if (rateControlMode == RATE_CR)
        return nullptr;
     timer++;
     failedCounter++;
     successCounter = 0;
     const IIeee80211Mode *slowerMode = nullptr;
     if (recovery) {
         if (retryCounter == 1) {
             reportRecoveryFailure();
             slowerMode = modeSet->getSlowerMode(dataFrameMode);
         }
         timer = 0;
     }
     else {
         if (needNormalFeedback) {
             reportFailure();
             slowerMode = modeSet->getSlowerMode(dataFrameMode);
         }
         if (retryCounter >= 2) {
             timer = 0;
         }
     }
     return slowerMode;
}

void Ieee80211MacAutoRate::reportFailure()
{
    if (rateControlMode == RATE_AARF) {
        setTimerTimeout(minTimerTimeout);
        setSuccessThreshold(minSuccessThreshold);
    }
}

void Ieee80211MacAutoRate::reportDataOk(const Ieee80211ModeSet *modeSet, const IIeee80211Mode *dataFrameMode)
{
    const IIeee80211Mode *fasterMode = computeFasterDataFrameMode(modeSet, dataFrameMode);
    if (fasterMode)
        ieee80211mac->setDataFrameMode(fasterMode);
}

void Ieee80211MacAutoRate::reportDataFailed(const Ieee80211ModeSet *modeSet, const IIeee80211Mode *dataFrameMode, unsigned int retryCounter, bool needNormalFeedback)
{
    const IIeee80211Mode *slowerMode = computeSlowerDataFrameMode(modeSet, dataFrameMode, retryCounter, needNormalFeedback);
    if (slowerMode)
        ieee80211mac->setDataFrameMode(slowerMode);
}

} /* namespace ieee80211 */
} /* namespace inet */

