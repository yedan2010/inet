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

#ifndef __INET_HCFCOLLISIONCONTROLLER_H
#define __INET_HCFCOLLISIONCONTROLLER_H

#include "inet/linklayer/ieee80211/mac/contract/ICollisionController.h"
#include "inet/linklayer/ieee80211/mac/contract/IContentionBasedChannelAccess.h"

namespace inet {
namespace ieee80211 {

class INET_API HcfCollisionController : public cSimpleModule, public ICollisionController
{
    protected:
        std::map<int, std::pair<IContentionBasedChannelAccess*,cMessage*>> timers;
        simtime_t timeLastProcessed;

    protected:
        virtual void handleMessage(cMessage* msg) override;
        virtual std::string getName(IContentionBasedChannelAccess *channelAccess);
        virtual int getPriority(IContentionBasedChannelAccess *channelAccess);
        virtual IContentionBasedChannelAccess* getHighestPriortyChannelAccess();

    public:
        virtual void scheduleTransmissionRequest(IContentionBasedChannelAccess *channelAccess, simtime_t txStartTime, ICallback *callback) override;
        virtual void cancelTransmissionRequest(IContentionBasedChannelAccess *channelAccess) override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef INET_HCFCOLLISIONCONTROLLER_H
