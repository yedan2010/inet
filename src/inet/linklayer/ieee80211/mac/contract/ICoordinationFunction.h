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

#ifndef __INET_ICOORDINATIONFUNCTION_H
#define __INET_ICOORDINATIONFUNCTION_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

/**
 * Interface for IEEE 802.11 Coordination Functions.
 */
class INET_API ICoordinationFunction
{
    public:
        virtual ~ICoordinationFunction() {}

        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) = 0;
        virtual void processLowerFrame(Ieee80211Frame *frame) = 0;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_ICOORDINATIONFUNCTION_H
