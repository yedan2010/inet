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

#include <FrameSequenceHandler.h>
#include "inet/linklayer/ieee80211/mac/contract/IContentionBasedChannelAccess.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access Function.
 */
class INET_API Edcaf : public IContentionBasedChannelAccess, public IContention::ICallback
{
    protected:
        IContention *contention = nullptr;
        IContentionBasedChannelAccess::ICallback *callback = nullptr;
        ICollisionController *collisionController = nullptr;

        bool owning = false;
        bool contentionInProgress = false;

        simtime_t slotTime = -1;
        simtime_t sifs = -1;
        simtime_t ifs = -1;
        simtime_t eifs = -1;
        simtime_t txopLimit = -1;

        AccessCategory ac = AccessCategory(-1);

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        AccessCategory getAccessCategory(const char *ac);
        int getAifsNumber();
        int getCwMax(int aCwMax, int aCwMin); // TODO: remove
        int getCwMin(int aCwMin);// TODO: remove

    public:
        virtual AccessCategory getAccessCategory() { return ac; }

        // IContentionBasedChannelAccess
        virtual void requestChannelAccess(IContentionBasedChannelAccess::ICallback *callback, int cw) override;
        virtual void releaseChannelAccess(IContentionBasedChannelAccess::ICallback *callback) override;
        virtual void channelAccessGranted() override;

        // IContention::ICallback
        virtual void txStartTimeCalculated(simtime_t txStartTime) override;
        virtual void txStartTimeCanceled() override;

        // Edcaf
        virtual bool isOwning() { return owning; }
        virtual bool isInternalCollision();
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_EDCAF_H
