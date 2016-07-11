//
// Copyright (C) 2016 OpenSim Ltd.
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

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Dcf.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/LegacyDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/framesequence/DcfFs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/linklayer/ieee80211/mac/lifetime/DcfTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(Dcf);

inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}

void Dcf::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        originatorMpduHandler = check_and_cast<OriginatorMpduHandler *>(getSubmodule("originatorMpduHandler"));
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        dcfChannelAccess = new DcfChannelAccess(rateSelection);
        const IIeee80211Mode *referenceMode = rateSelection->getSlowestMandatoryMode(); // or any other; slotTime etc must be the same for all modes we use
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        // rx->registerContention(contention);
    }
}

void Dcf::channelAccessGranted()
{
    frameSequenceHandler->startFrameSequence(new DcfFs(), context, this);
}

void Dcf::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    originatorMpduHandler->processUpperFrame(frame);
    dcfChannelAccess->requestChannelAccess(this);
}

void Dcf::processLowerFrame(Ieee80211Frame* frame)
{
    if (frameSequenceHandler->isSequenceRunning())
        frameSequenceHandler->processResponse(frame);
    else
        recipientMpduHandler->processReceivedFrame(frame);
}

void Dcf::transmitFrame(Ieee80211Frame* frame, simtime_t ifs)
{
    tx->transmitFrame(frame, ifs, this);
}

void Dcf::frameSequenceFinished()
{
    if (originatorMpduHandler->hasFrameToTransmit())
        dcfChannelAccess->requestChannelAccess(this);
}

bool Dcf::isReceptionInProgress()
{
    return rx->isReceptionInProgress();
}

void Dcf::transmissionComplete()
{
    frameSequenceHandler->transmissionComplete();
}

bool Dcf::hasFrameToTransmit()
{
}

void Dcf::processRtsProtectionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
}

void Dcf::processTransmittedFrame(Ieee80211Frame* transmittedFrame)
{
}

void Dcf::processReceivedFrame(Ieee80211Frame* frame, Ieee80211Frame* lastTransmittedFrame)
{
}

void Dcf::processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame)
{
}

} // namespace ieee80211
} // namespace inet

