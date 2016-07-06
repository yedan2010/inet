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

#include "HcfCollisionController.h"
#include "inet/linklayer/ieee80211/mac/common/AccessCategory.h"

namespace inet {
namespace ieee80211 {

std::string HcfCollisionController::getName(IContentionBasedChannelAccess* channelAccess)
{
    // TODO:
}

int HcfCollisionController::getPriority(IContentionBasedChannelAccess* channelAccess)
{
    if (auto edcaf = dynamic_cast<Edcaf *>(channelAccess))
        return edcaf->getAccessCategory() + 1;
    else
       return 0;
}

void HcfCollisionController::scheduleTransmissionRequest(IContentionBasedChannelAccess *channelAccess, simtime_t txStartTime, ICallback *cb)
{
    // TODO: Enter_Method("scheduleTransmissionRequest(%d)", txIndex);
    ASSERT(txStartTime > timeLastProcessed); // if equal then it's too late, that round was already done and notified (check timer's scheduling priority if that happens!)
    int priority = getPriority(channelAccess);
    if (timers.find(priority) != timers.end()) {
        char name[16];
        // TODO: sprintf(name, "txStart-%d", txIndex);
        timers[priority] = std::make_pair(channelAccess, new cMessage(name));
        timers[priority]->setSchedulingPriority(1000); // low priority, i.e. processed later than most events for the same time
    }
    ASSERT(!timers[priority]->isScheduled());
    scheduleAt(txStartTime, timers[priority]);
}

void HcfCollisionController::cancelTransmissionRequest(IContentionBasedChannelAccess *channelAccess)
{
    // TODO: Enter_Method("cancelTransmissionRequest(%d)", txIndex);
    int priority = getPriority(channelAccess);
    ASSERT(timers.find(priority) != timers.end());
    cancelEvent(timers[priority]);
}

void HcfCollisionController::handleMessage(cMessage *msg)
{
    // from the ones scheduled for the current simulation time: grant transmission to the
    // highest priority one, and signal internal collision to the others
    simtime_t now = simTime();
    bool granted = false;
    for (auto it = timers.rbegin(); it != timers.rend(); it++) {
        auto channelAccess = (*it).second.first;
        auto timer = (*it).second.second;
        if (timer->getArrivalTime() == now) {
            if (!granted) {
                channelAccess->transmissionGranted();
                granted = true;
            }
            else
                channelAccess->internalCollision();
        }
    }
    timeLastProcessed = now;
}


} /* namespace ieee80211 */
} /* namespace inet */
