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

#ifndef __INET_DCFCHANNELACCESS_H
#define __INET_DCFCHANNELACCESS_H

#include "inet/linklayer/ieee80211/mac/contract/IContentionBasedChannelAccess.h"

namespace inet {
namespace ieee80211 {

class INET_API DcfChannelAccess : public IContentionBasedChannelAccess, public IContention::ICallback
{
    protected:
        IContention *contention = nullptr;
        IContentionBasedChannelAccess::ICallback *callback = nullptr;

        bool owning = false;
        bool contentionInProgress = false;

        simtime_t slotTime = -1;
        simtime_t sifs = -1;
        simtime_t ifs = -1;
        simtime_t eifs = -1;

    public:
        DcfChannelAccess(IRateSelection *rateSelection);

        // IContentionBasedChannelAccess
        virtual void channelAccessGranted() override;
        virtual void requestChannelAccess(IContentionBasedChannelAccess::ICallback* callback, int cw) override;
        virtual void releaseChannelAccess(IContentionBasedChannelAccess::ICallback* callback) override;

        // IContention::ICallback
        virtual void txStartTimeCalculated(simtime_t txStartTime) override;
        virtual void txStartTimeCanceled() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_DCFCHANNELACCESS_H
