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

#ifndef INET_ORIGINATORBLOCKACKAGREEMENT_H
#define INET_ORIGINATORBLOCKACKAGREEMENT_H

#include "inet/common/INETDefs.h"

namespace inet {
namespace ieee80211 {

class OriginatorBlockAckAgreement
{
    protected:
        int numSentBaPolicyFrames = 0;
        int bufferSize = -1;
        bool isAMsduSupported = false;
        bool isDelayedBlockAckPolicySupported = false;
        bool isAddbaResponseReceived = false;
        simtime_t blockAckTimeoutValue = -1;

    public:
        OriginatorBlockAckAgreement(int bufferSize, bool isAMsduSupported, bool isDelayedBlockAckPolicySupported) :
            bufferSize(bufferSize),
            isAMsduSupported(isAMsduSupported),
            isDelayedBlockAckPolicySupported(isDelayedBlockAckPolicySupported)
        { }

        int getBufferSize() const { return bufferSize; }
        bool getIsAddbaResponseReceived() const { return isAddbaResponseReceived; }
        bool getIsAMsduSupported() const { return isAMsduSupported; }
        bool getIsDelayedBlockAckPolicySupported() const { return isDelayedBlockAckPolicySupported; }
        const simtime_t getBlockAckTimeoutValue() const  { return blockAckTimeoutValue; }
        int getNumSentBaPolicyFrames() const { return numSentBaPolicyFrames; }

        void setBufferSize(int bufferSize) { this->bufferSize = bufferSize; }
        void setIsAddbaResponseReceived(bool isAddbaResponseReceived) { this->isAddbaResponseReceived = isAddbaResponseReceived; }
        void setIsAMsduSupported(bool isAMsduSupported) { this->isAMsduSupported = isAMsduSupported; }
        void setIsDelayedBlockAckPolicySupported(bool isDelayedBlockAckPolicySupported) { this->isDelayedBlockAckPolicySupported = isDelayedBlockAckPolicySupported; }
        void setBlockAckTimeoutValue(const simtime_t blockAckTimeoutValue) { this->blockAckTimeoutValue = blockAckTimeoutValue; }

        void baPolicyFrameSent() { numSentBaPolicyFrames++; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // INET_ORIGINATORBLOCKACKAGREEMENT_H
