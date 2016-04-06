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

#include "BlockAckAgreement.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientBlockAckAgreementHandler.h"

namespace inet {
namespace ieee80211 {

BlockAckAgreement::BlockAckAgreement(RecipientBlockAckAgreementHandler *blockAckHandler, MACAddress originatorAddress, Tid tid, SequenceNumber startingSequenceNumber, int bufferSize, simtime_t lastUsedTime) :
    agreementHandler(blockAckHandler),
    startingSequenceNumber(startingSequenceNumber),
    bufferSize(bufferSize),
    blockAckTimeoutValue(lastUsedTime)
{
    blockAckRecord = new BlockAckRecord(originatorAddress, tid);
    inactivityTimer = new cMessage("Inactivity Timer");
    scheduleInactivityTimer();
}

BlockAckAgreement::~BlockAckAgreement()
{
    agreementHandler->cancelAndDelete(inactivityTimer);
}

void BlockAckAgreement::blockAckPolicyFrameReceived(Ieee80211DataFrame* frame)
{
    ASSERT(frame->getAckPolicy() == BLOCK_ACK);
    blockAckRecord->blockAckPolicyFrameReceived(frame);
}

//
// Every STA shall maintain an inactivity timer for every negotiated Block Ack setup. The inactivity timer at a
// recipient is reset when MPDUs corresponding to the TID for which the Block Ack policy is set are received
// and the Ack Policy subfield in the QoS Control field of that MPDU header is Block Ack or Implicit Block
// Ack Request.
//
void BlockAckAgreement::scheduleInactivityTimer()
{
    if (blockAckTimeoutValue != 0) {
        agreementHandler->cancelEvent(inactivityTimer);
        agreementHandler->scheduleAt(simTime() + blockAckTimeoutValue, inactivityTimer);
    }
}

} /* namespace ieee80211 */
} /* namespace inet */
