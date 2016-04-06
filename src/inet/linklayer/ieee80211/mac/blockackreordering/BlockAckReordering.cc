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

#include "BlockAckReordering.h"
#include "inet/linklayer/ieee80211/mac/blockack/BlockAckAgreement.h"


namespace inet {
namespace ieee80211 {

//
// The recipient flushes received MSDUs from its receive buffer as described in this subclause. [...]
//
BlockAckReordering::ReorderBuffer BlockAckReordering::processReceivedQoSFrame(BlockAckAgreement *agreement, Ieee80211DataFrame* dataFrame)
{
    MACAddress originatorAddr = dataFrame->getTransmitterAddress();
    Tid tid = dataFrame->getTid();
    auto id = std::make_pair(tid, originatorAddr);
    auto it = receiveBuffers.find(id);
    if (it != receiveBuffers.end()) {
        ReceiveBuffer *receiveBuffer = it->second;
        // The reception of QoS data frames using Normal Ack policy shall not be used by the
        // recipient to reset the timer to detect Block Ack timeout (see 10.5.4).
        // This allows the recipient to delete the Block Ack if the originator does not switch
        // back to using Block Ack.
        if (dataFrame->getAckPolicy() == BLOCK_ACK) // TODO: Implicit Block Ack
            agreement->scheduleInactivityTimer();
        if (receiveBuffer->insertFrame(dataFrame)) {
            if (dataFrame->getAckPolicy() == BLOCK_ACK)
                agreement->blockAckPolicyFrameReceived(dataFrame);
            auto earliestCompleteMsduOrAMsdu = getEarliestCompleteMsduOrAMsduIfExists(receiveBuffer);
            if (earliestCompleteMsduOrAMsdu.size() > 0) {
                int earliestSequenceNumber = earliestCompleteMsduOrAMsdu.at(0)->getSequenceNumber();
                // If, after an MPDU is received, the receive buffer is full, the complete MSDU or A-MSDU with the earliest
                // sequence number shall be passed up to the next MAC process.
                if (receiveBuffer->isFull()) {
                    passedUp(receiveBuffer, earliestSequenceNumber);
                    return ReorderBuffer({std::make_pair(earliestSequenceNumber, Fragments({earliestCompleteMsduOrAMsdu}))});
                }
                // If, after an MPDU is received, the receive buffer is not full, but the sequence number of the complete MSDU or
                // A-MSDU in the buffer with the lowest sequence number is equal to the NextExpectedSequenceNumber for
                // that Block Ack agreement, then the MPDU shall be passed up to the next MAC process.
                else if (earliestSequenceNumber == receiveBuffer->getNextExpectedSequenceNumber()) {
                    int seqNum = dataFrame->getSequenceNumber();
                    passedUp(receiveBuffer, seqNum);
                    return ReorderBuffer({std::make_pair(seqNum, Fragments({dataFrame}))});
                }
            }
        }
    }
    else {
        throw cRuntimeError("Receive buffer is not found");
    }
}

//
// The recipient flushes received MSDUs from its receive buffer as described in this subclause. [...]
//
BlockAckReordering::ReorderBuffer BlockAckReordering::processReceivedBlockAckReq(Ieee80211BlockAckReq* frame)
{
    // The originator shall use the Block Ack starting sequence control to signal the first MPDU in the block for
    // which an acknowledgment is expected.
    int startingSequenceNumber = -1;
    Tid tid = -1;
    if (auto basicReq = dynamic_cast<Ieee80211BasicBlockAckReq*>(frame)) {
        tid = basicReq->getTidInfo();
        startingSequenceNumber = basicReq->getStartingSequenceNumber();
    }
    else if (auto compressedReq = dynamic_cast<Ieee80211CompressedBlockAck*>(frame)) {
        tid = compressedReq->getTidInfo();
        startingSequenceNumber = compressedReq->getStartingSequenceNumber();
    }
    else {
        throw cRuntimeError("Multi-Tid BlockAckReq is currently an unimplemented feature");
    }
    auto id = std::make_pair(tid, frame->getTransmitterAddress());
    auto it = receiveBuffers.find(id);
    if (it != receiveBuffers.end()) {
        ReceiveBuffer *receiveBuffer = it->second;
        // MPDUs in the recipient’s buffer with a sequence control value that
        // precedes the starting sequence control value are called preceding MPDUs.
        // The recipient shall reassemble any complete MSDUs from buffered preceding
        // MPDUs and indicate these to its higher layer.
        auto completePrecedingMpdus = collectCompletePrecedingMpdus(receiveBuffer, startingSequenceNumber);
        // Upon arrival of a BlockAckReq frame, the recipient shall pass up the MSDUs and A-MSDUs starting with
        // the starting sequence number sequentially until there is an incomplete or missing MSDU
        // or A-MSDU in the buffer.
        auto consecutiveCompleteFollowingMpdus = collectConsecutiveCompleteFollowingMpdus(receiveBuffer, startingSequenceNumber);
        // If no MSDUs or A-MSDUs are passed up to the next MAC process after the receipt
        // of the BlockAckReq frame and the starting sequence number of the BlockAckReq frame is newer than the
        // NextExpectedSequenceNumber for that Block Ack agreement, then the NextExpectedSequenceNumber for
        // that Block Ack agreement is set to the sequence number of the BlockAckReq frame.
        int numOfMsdusToPassUp = completePrecedingMpdus.size() + consecutiveCompleteFollowingMpdus.size();
        if (numOfMsdusToPassUp == 0  && receiveBuffer->getNextExpectedSequenceNumber() < startingSequenceNumber)
            receiveBuffer->setNextExpectedSequenceNumber(startingSequenceNumber);
        // The recipient shall then release any buffers held by preceding MPDUs.
        releaseReceiveBuffer(receiveBuffer, completePrecedingMpdus);
        releaseReceiveBuffer(receiveBuffer, consecutiveCompleteFollowingMpdus);
        // The recipient shall pass MSDUs and A-MSDUs up to the next MAC process in order of increasing sequence
        // number.
        completePrecedingMpdus.insert(consecutiveCompleteFollowingMpdus.begin(), consecutiveCompleteFollowingMpdus.end());
        return completePrecedingMpdus;
    }
    return ReorderBuffer();
}

//
// If a BlockAckReq frame is received, all complete MSDUs and A-MSDUs with lower sequence numbers than
// the starting sequence number contained in the BlockAckReq frame shall be passed up to the next MAC process
// as shown in Figure 5-1.
//
BlockAckReordering::ReorderBuffer BlockAckReordering::collectCompletePrecedingMpdus(ReceiveBuffer *receiveBuffer, int startingSequenceNumber)
{
    ASSERT(startingSequenceNumber != -1);
    ReorderBuffer completePrecedingMpdus;
    const auto& buffer = receiveBuffer->getBuffer();
    for (auto it : buffer) { // collects complete preceding MPDUs
        int sequenceNumber = it.first;
        auto fragments = it.second;
        if (sequenceNumber < startingSequenceNumber && isComplete(fragments))
            completePrecedingMpdus[sequenceNumber] = fragments;
    }
    return completePrecedingMpdus;
}

//
// Upon arrival of a BlockAckReq frame, the recipient shall pass up the MSDUs and A-MSDUs starting with
// the starting sequence number sequentially until there is an incomplete or missing MSDU
// or A-MSDU in the buffer.
//
BlockAckReordering::ReorderBuffer BlockAckReordering::collectConsecutiveCompleteFollowingMpdus(ReceiveBuffer *receiveBuffer, int startingSequenceNumber)
{
    ASSERT(startingSequenceNumber != -1);
    ReorderBuffer framesToPassUp;
    const auto& buffer = receiveBuffer->getBuffer();
    for (int i = startingSequenceNumber; i < startingSequenceNumber + receiveBuffer->getBufferSize(); i++) {
        auto it = buffer.find(i);
        if (it != buffer.end()) {
            int sequenceNumber = it->first;
            auto fragments = it->second;
            if (isComplete(fragments))
                framesToPassUp[sequenceNumber] = fragments;
            else
                return framesToPassUp; // incomplete
        }
        else
            return framesToPassUp; // missing
    }
    return framesToPassUp;
}


void BlockAckReordering::releaseReceiveBuffer(ReceiveBuffer *receiveBuffer, const ReorderBuffer& reorderBuffer)
{
    for (auto it : reorderBuffer) {
        int sequenceNumber = it.first;
        passedUp(receiveBuffer, sequenceNumber);
    }
}


bool BlockAckReordering::isComplete(const std::vector<Ieee80211DataFrame*>& fragments)
{
    int largestFragmentNumber = -1;
    std::set<FragmentNumber> fragNums; // possible duplicate frames
    for (auto fragment : fragments) {
        if (!fragment->getMoreFragments())
            largestFragmentNumber = fragment->getFragmentNumber();
        fragNums.insert(fragment->getFragmentNumber());
    }
    return largestFragmentNumber != -1 && largestFragmentNumber + 1 == (int)fragNums.size();
}

void BlockAckReordering::agreementEstablished(BlockAckAgreement* agreement)
{
    int bufferSize = agreement->getBufferSize();
    Tid tid = agreement->getBlockAckRecord()->getTid();
    MACAddress originatorAddr = agreement->getBlockAckRecord()->getOriginatorAddress();
    auto id = std::make_pair(tid, originatorAddr);
    receiveBuffers[id] = new ReceiveBuffer(bufferSize);
}

void BlockAckReordering::agreementTerminated(BlockAckAgreement* agreement)
{
    Tid tid = agreement->getBlockAckRecord()->getTid();
    MACAddress originatorAddr = agreement->getBlockAckRecord()->getOriginatorAddress();
    auto id = std::make_pair(tid, originatorAddr);
    auto it = receiveBuffers.find(id);
    if (it != receiveBuffers.end()) {
        delete it->second;
        receiveBuffers.erase(it);
    }
    else {
        throw cRuntimeError("Receive buffer is not found");
    }
}

void BlockAckReordering::passedUp(ReceiveBuffer *receiveBuffer, int sequenceNumber)
{
    // Each time that the recipient passes an MSDU or A-MSDU for a Block Ack agreement up to the next MAC
    // process, the NextExpectedSequenceNumber for that Block Ack agreement is set to the sequence number of the
    // MSDU or A-MSDU that was passed up to the next MAC process plus one.
    receiveBuffer->setNextExpectedSequenceNumber(sequenceNumber + 1);
    receiveBuffer->remove(sequenceNumber);
}

std::vector<Ieee80211DataFrame*> BlockAckReordering::getEarliestCompleteMsduOrAMsduIfExists(ReceiveBuffer *receiveBuffer)
{
    const auto& buffer = receiveBuffer->getBuffer();
    for (auto it : buffer) {
        auto &fragments = it.second;
        if (isComplete(fragments))
            return fragments;
    }
    return Fragments();
}

} /* namespace ieee80211 */
} /* namespace inet */
