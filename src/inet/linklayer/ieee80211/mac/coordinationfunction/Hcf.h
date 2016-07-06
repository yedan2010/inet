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

#ifndef __INET_HCF_H
#define __INET_HCF_H

#include "inet/linklayer/ieee80211/mac/contract/IHybridCoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Edca.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcca.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Hybrid Coordination Function.
 */
class INET_API Hcf : public IHybridCoordinationFunction
{
    protected:
        Edca *edca = nullptr;
        Hcca *hcca = nullptr; // TODO: unimplemented

    public:
        Hcf(Edca *edca, Hcca *hcca);

        virtual bool isSequenceRunning() { return edca->isSequenceRunning(); }

        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void processLowerFrame(Ieee80211Frame *frame) override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_HCF_H
