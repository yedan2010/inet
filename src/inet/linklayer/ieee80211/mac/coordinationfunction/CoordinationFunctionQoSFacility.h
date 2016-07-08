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

#ifndef __INET_COORDINATIONFUNCTIONQOSFACILITY_H
#define __INET_COORDINATIONFUNCTIONQOSFACILITY_H

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Dcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Mcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Pcf.h"

namespace inet {
namespace ieee80211 {

class INET_API CoordinationFunctionQoSFacility
{
    protected:
        Hcf *hcf = nullptr;

        ICollisionController *collisionController = nullptr;
        RecipientQoSMpduHandler *recipientMpduHandler = nullptr;

    public:
        CoordinationFunctionQoSFacility(Hcf *hcf, RecipientQoSMpduHandler *recipientMpduHandler, ICollisionController *collisionController) :
            hcf(hcf),
            recipientMpduHandler(recipientMpduHandler),
            collisionController(collisionController)
        { }

        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual void processLowerFrame(Ieee80211Frame *frame);
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_COORDINATIONFUNCTIONQOSFACILITY_H
