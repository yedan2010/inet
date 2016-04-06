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

#ifndef __INET_BLOCKACKAGREEMENT_H
#define __INET_BLOCKACKAGREEMENT_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/BlockAckRecord.h"

namespace inet {
namespace ieee80211 {

class RecipientBlockAckAgreementHandler;

class INET_API BlockAckAgreement // TODO: RecipientBlockAckAgreement
{
    protected:
        RecipientBlockAckAgreementHandler *agreementHandler = nullptr;
        BlockAckRecord *blockAckRecord = nullptr;

        SequenceNumber startingSequenceNumber = -1;
        int bufferSize = -1;
        simtime_t blockAckTimeoutValue = 0;
        cMessage *inactivityTimer = nullptr;

        bool isAddbaResponseSent = false;

    public:
        BlockAckAgreement(RecipientBlockAckAgreementHandler *blockAckHandler, MACAddress originatorAddress, Tid tid, SequenceNumber startingSequenceNumber, int bufferSize, simtime_t blockAckTimeoutValue);
        ~BlockAckAgreement();

        void blockAckPolicyFrameReceived(Ieee80211DataFrame *frame);
        void scheduleInactivityTimer();

        BlockAckRecord *getBlockAckRecord() { return blockAckRecord; }
        cMessage *getInactivityTimer() { return inactivityTimer; }
        simtime_t getBlockAckTimeoutValue() { return blockAckTimeoutValue; }
        int getBufferSize() { return bufferSize; }
        int getStartingSequenceNumber() { return startingSequenceNumber; }

        void addbaResposneSent() { isAddbaResponseSent = true; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_BLOCKACKAGREEMENT_H
