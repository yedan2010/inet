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

#include "DcfChannelAccess.h"

namespace inet {
namespace ieee80211 {

DcfChannelAccess::DcfChannelAccess(IRateSelection *rateSelection)
{
    slotTime = referenceMode->getSlotTime();
    sifs = referenceMode->getSifsTime();
    ifs = fallback(par("difsTime"), sifs + 2 * slotTime);
    eifs = sifs + ifs + referenceMode->getDuration(LENGTH_ACK);
}

void DcfChannelAccess::channelAccessGranted()
{
    ASSERT(coordinationFunction != nullptr);
    callback->channelAccessGranted();
    owning = true;
    contentionInProgress = false;
}

void DcfChannelAccess::releaseChannelAccess(IContentionBasedChannelAccess::ICallback* callback)
{
    owning = false;
    contentionInProgress = false;
    this->callback = nullptr;
}

void DcfChannelAccess::requestChannelAccess(IContentionBasedChannelAccess::ICallback* callback, int cw)
{
    this->callback = callback;
    if (owning)
        callback->channelAccessGranted();
    else if (!contentionInProgress) {
        contention->startContention(cw);
        contentionInProgress = true;
    }
    else ;
}

void DcfChannelAccess::txStartTimeCalculated(simtime_t txStartTime)
{
    // don't care
}

void DcfChannelAccess::txStartTimeCanceled()
{
    // don't care
}

} /* namespace ieee80211 */
} /* namespace inet */

