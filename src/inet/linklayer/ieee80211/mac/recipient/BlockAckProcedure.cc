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

#include "BlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/blockack/BlockAckAgreement.h"

namespace inet {
namespace ieee80211 {

BlockAckProcedure::BlockAckProcedure(RecipientBlockAckAgreementHandler* agreementHandler, IRateSelection *rateSelection)
{
    this->agreementHandler = agreementHandler;
    this->rateSelection = rateSelection;
    this->sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
}


void BlockAckProcedure::processReceivedBlockAckReq(Ieee80211BlockAckReq* blockAckReq)
{
 // TODO??
}

void BlockAckProcedure::processTransmittedBlockAck(Ieee80211BlockAck* blockAck)
{
// TODO??
}

//
// The Basic BlockAck frame contains acknowledgments for the MPDUs of up to 64 previous MSDUs. In the
// Basic BlockAck frame, the STA acknowledges only the MPDUs starting from the starting sequence control
// until the MPDU with the highest sequence number that has been received, and the STA shall set bits in the
// Block Ack bitmap corresponding to all other MPDUs to 0.
//
Ieee80211BlockAck* BlockAckProcedure::buildBlockAck(Ieee80211BlockAckReq* blockAckReq)
{
    if (auto basicBlockAckReq = dynamic_cast<Ieee80211BasicBlockAckReq*>(blockAckReq)) {
        Tid tid = basicBlockAckReq->getTidInfo();
        MACAddress originatorAddr = basicBlockAckReq->getTransmitterAddress();
        BlockAckAgreement *agreement = agreementHandler->getAgreement(tid, originatorAddr);
        if (agreement) {
            Ieee80211BasicBlockAck *blockAck = new Ieee80211BasicBlockAck("BasicBlockAck");
            int startingSequenceNumber = basicBlockAckReq->getStartingSequenceNumber();
            for (SequenceNumber seqNum = startingSequenceNumber; seqNum < startingSequenceNumber + 64; seqNum++) {
                BitVector &bitmap = blockAck->getBlockAckBitmap(seqNum - startingSequenceNumber);
                for (FragmentNumber fragNum = 0; fragNum < 16; fragNum++) {
                    bool ackState = agreement->getBlockAckRecord()->getAckState(seqNum, fragNum);
                    bitmap.setBit(fragNum, ackState);
                }
            }
            // FIXME: blockAck->setTransmitterAddress(params->getAddress());
            blockAck->setReceiverAddress(blockAckReq->getTransmitterAddress());
            blockAck->setDuration(0);
            blockAck->setCompressedBitmap(false);
            return blockAck;
        }
        else {
            return nullptr;
        }
    }
    else {
        throw cRuntimeError("Unsupported Block Ack Request");
    }
}

} /* namespace ieee80211 */
} /* namespace inet */
