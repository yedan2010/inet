//
// Copyright (C) 2016 OpenSim Ltd.
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
//

#include "Edcaf.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/QosDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/framesequence/HcfFs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/linklayer/ieee80211/mac/lifetime/EdcaTransmitLifetimeHandler.h"

namespace inet {
namespace ieee80211 {

inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}

//Edcaf::Edcaf(int aifsn, AccessCategory ac, IChannelAccess::ICallback *callback, IEdcaCollisionController *collisionController, RecoveryProcedure *recoveryProcedure, IRateSelection *rateSelection)
//{
//    this->ac = ac;
//    this->callback = callback;
//    this->collisionController = collisionController;
//    this->recoveryProcedure = recoveryProcedure;
//    auto modeSet = rateSelection->getModeSet();
//    auto referenceMode = modeSet->getSlowerMandatoryMode();
//    slotTime = referenceMode->getSlotTime();
//    sifs = referenceMode->getSifsTime();
//    simtime_t aifs = sifs + fallback(aifsn, getAifsNumber(ac)) * slotTime;
//    ifs = aifs;
//    eifs = sifs + aifs + referenceMode->getDuration(LENGTH_ACK);
//}

int Edcaf::getAifsNumber(AccessCategory ac)
{
    switch (ac)
    {
        case AC_BK: return 7;
        case AC_BE: return 3;
        case AC_VI: return 2;
        case AC_VO: return 2;
        default: throw cRuntimeError("Unknown access category = %d", ac);
    }
}

// TODO: remove
int Edcaf::getCwMax(int aCwMax, int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMax;
        case AC_BE: return aCwMax;
        case AC_VI: return aCwMin;
        case AC_VO: return (aCwMin + 1) / 2 - 1;
        default: throw cRuntimeError("Unknown access category = %d", ac);
    }
}

// TODO: remove
int Edcaf::getCwMin(int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMin;
        case AC_BE: return aCwMin;
        case AC_VI: return (aCwMin + 1) / 2 - 1;
        case AC_VO: return (aCwMin + 1) / 4 - 1;
        default: throw cRuntimeError("Unknown access category = %d", ac);
    }
}

AccessCategory Edcaf::getAccessCategory(const char *ac)
{
    if (strcmp("AC_BK", ac) == 0)
        return AC_BK;
    if (strcmp("AC_VI", ac) == 0)
        return AC_VI;
    if (strcmp("AC_VO", ac) == 0)
        return AC_VO;
    if (strcmp("AC_BE", ac) == 0)
        return AC_BE;
    throw cRuntimeError("Unknown Access Category = %s", ac);
}

void Edcaf::channelAccessGranted()
{
    ASSERT(callback != nullptr);
    if (!collisionController->isInternalCollision(this)) {
        callback->channelAccessGranted();
        owning = true;
    }
    contentionInProgress = false;
}

void Edcaf::releaseChannelAccess(IChannelAccess::ICallback* callback)
{
    ASSERT(owning);
    owning = false;
    contentionInProgress = false;
    this->callback = nullptr;
}

void Edcaf::requestChannelAccess(IChannelAccess::ICallback* callback)
{
    this->callback = callback;
    if (owning)
        callback->channelAccessGranted();
    else if (!contentionInProgress) {
        contention->startContention(recoveryProcedure->getCw(), ifs, eifs, slotTime, this);
        contentionInProgress = true;
    }
    else ;
}

void Edcaf::txStartTimeCalculated(simtime_t txStartTime)
{
    collisionController->recordTxStartTime(this, txStartTime);
}

void Edcaf::txStartTimeCanceled()
{
    collisionController->cancelTxStartTime(this);
}

bool Edcaf::isInternalCollision()
{
    return collisionController->isInternalCollision(this);
}

} // namespace ieee80211
} // namespace inet
