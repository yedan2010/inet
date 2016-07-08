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

#include "EdcaCollisionController.h"
#include "inet/linklayer/ieee80211/mac/common/AccessCategory.h"

namespace inet {
namespace ieee80211 {

EdcaCollisionController::EdcaCollisionController()
{
    for (int ac = 0; ac < 4; ac++)
        txStartTimes[ac] = -1;
}

void EdcaCollisionController::scheduleTransmissionRequest(Edcaf *edcaf, simtime_t txStartTime)
{
    Enter_Method("scheduleTransmissionRequest(%d)", edcaf->getAccessCategory());
    txStartTimes[edcaf->getAccessCategory()] = txStartTime;
}

void EdcaCollisionController::cancelTransmissionRequest(Edcaf *edcaf)
{
    Enter_Method("cancelTransmissionRequest(%d)", edcaf->getAccessCategory());
    txStartTimes[edcaf->getAccessCategory()] = -1;
}

bool EdcaCollisionController::isInternalCollision(Edcaf *edcaf)
{
    simtime_t now = simTime();
    for (int ac = 0; ac < 4; ac++) {
        if (txStartTimes[i] == now && edcaf->getAccessCategory() < ac)
            return true;
    }
    return false;
}

} /* namespace ieee80211 */
} /* namespace inet */
