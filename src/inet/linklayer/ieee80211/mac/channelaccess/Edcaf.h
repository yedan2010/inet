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

#ifndef __INET_EDCAF_H
#define __INET_EDCAF_H

#include "inet/linklayer/ieee80211/mac/contract/IChannelAccess.h"
#include "inet/linklayer/ieee80211/mac/contract/IContention.h"
#include "inet/linklayer/ieee80211/mac/contract/IEdcaCollisionController.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access Function.
 */
class INET_API Edcaf : public IChannelAccess, public IContention::ICallback
{
    protected:
        RecoveryProcedure *recoveryProcedure = nullptr;
        IContention *contention = nullptr;
        IChannelAccess::ICallback *callback = nullptr;
        IEdcaCollisionController *collisionController = nullptr;

        bool owning = false;
        bool contentionInProgress = false;

        simtime_t slotTime = -1;
        simtime_t sifs = -1;
        simtime_t ifs = -1;
        simtime_t eifs = -1;
        simtime_t txopLimit = -1;

        AccessCategory ac = AccessCategory(-1);

    protected:

        AccessCategory getAccessCategory(const char *ac);
        int getCwMax(int aCwMax, int aCwMin); // TODO: remove
        int getCwMin(int aCwMin);// TODO: remove

        virtual int getAifsNumber(AccessCategory ac);

    public:
        //Edcaf(int aifsn, AccessCategory ac, IChannelAccess::ICallback *callback, IEdcaCollisionController *collisionController, RecoveryProcedure *recoveryProcedure, IRateSelection *rateSelection);

        virtual AccessCategory getAccessCategory() { return ac; }

        // IContentionBasedChannelAccess
        virtual void requestChannelAccess(IChannelAccess::ICallback *callback) override;
        virtual void releaseChannelAccess(IChannelAccess::ICallback *callback) override;

        // IContention::ICallback
        virtual void channelAccessGranted() override;
        virtual void txStartTimeCalculated(simtime_t txStartTime) override;
        virtual void txStartTimeCanceled() override;

        // Edcaf
        virtual bool isOwning() { return owning; }
        virtual bool isInternalCollision();
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_EDCAF_H
