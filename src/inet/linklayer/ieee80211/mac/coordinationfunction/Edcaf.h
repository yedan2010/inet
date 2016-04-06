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

#ifndef __INET_EDCAF_H
#define __INET_EDCAF_H

#include "inet/linklayer/ieee80211/mac/contract/ICoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/CafBase.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access Function.
 */
class INET_API Edcaf : public CafBase, public ICoordinationFunction, public IContention::ICallback
{
    protected:
        AccessCategory ac = AccessCategory(-1);
        simtime_t txopLimit = -1;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES;}
        virtual void initialize(int stage) override;

        AccessCategory getAccessCategory(const char *ac);
        int getAifsNumber();
        int getCwMax(int aCwMax, int aCwMin); // TODO: remove
        int getCwMin(int aCwMin);// TODO: remove

    public:
        virtual void channelAccessGranted() override;
        virtual void internalCollision() override;

        virtual AccessCategory getAccessCategory() { return ac; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_EDCAF_H
