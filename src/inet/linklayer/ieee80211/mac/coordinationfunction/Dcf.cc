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
    CafBase::initialize(stage);
    if (stage == INITSTAGE_LINK_LAYER_2) {
        originatorMpduHandler = check_and_cast<IOriginatorMpduHandler *>(getSubmodule("originatorMpduHandler"));
        contentionCallback = this;
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        const IIeee80211Mode *referenceMode = rateSelection->getSlowestMandatoryMode(); // or any other; slotTime etc must be the same for all modes we use
        slotTime = referenceMode->getSlotTime();
        sifs = referenceMode->getSifsTime();
        ifs = fallback(par("difsTime"), sifs + 2 * slotTime);
        eifs = sifs + ifs + referenceMode->getDuration(LENGTH_ACK);
        contention->setTxIndex(0); // FIXME
    }
}

void Dcf::channelAccessGranted()
{
    if (!isSequenceRunning()) {
        frameSequence = new DcfFs();
        context = originatorMpduHandler->buildContext();
        frameSequence->startSequence(context, 0);
        startFrameSequenceStep();
    }
    else
        throw cRuntimeError("Channel access granted while a frame sequence is running");
}

void Dcf::internalCollision()
{
    // FIXME: implement this
    throw cRuntimeError("IMPLEMENT");
}

} // namespace ieee80211
} // namespace inet

