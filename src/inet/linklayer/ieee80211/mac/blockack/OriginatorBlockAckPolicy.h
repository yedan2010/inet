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

#ifndef __INET_ORIGINATORBLOCKACKPOLICY_H
#define __INET_ORIGINATORBLOCKACKPOLICY_H

#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"

namespace inet {
namespace ieee80211 {

enum class BaPolicyAction {
    SEND_ADDBA_REQUEST,
    SEND_BA_REQUEST,
    SEND_WITH_BLOCK_ACK,
    SEND_WITH_NORMAL_ACK
};

class INET_API OriginatorBlockAckPolicy
{
    protected:
        BaPolicyAction getAckPolicy(Ieee80211DataFrame* frame, OriginatorBlockAckAgreement *agreement);
        bool isEligibleFrame(Ieee80211DataFrame* frame, OriginatorBlockAckAgreement *agreement);

    public:
        BaPolicyAction getAction(FrameSequenceContext *context);
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_ORIGINATORBLOCKACKPOLICY_H
