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

#include "OriginatorBlockAckPolicy.h"

namespace inet {
namespace ieee80211 {

BaPolicyAction OriginatorBlockAckPolicy::getAction(FrameSequenceContext *context)
{
    Ieee80211Frame *frameToTransmit = context->getInProgressFrames()->getFrameToTransmit();
    auto outstandingFramesPerReceiver = context->getOutstandingFramesPerReceiver();
    for (auto outstandingFrames : outstandingFramesPerReceiver) {
        if (outstandingFrames.second.size() >= 5)
            return BaPolicyAction::SEND_BA_REQUEST;
    }
    if (frameToTransmit->getByteLength() > 1000) {
        return BaPolicyAction::SEND_WITH_NORMAL_ACK;
    }
    else {
        Ieee80211DataFrame *dataFrameToTransmit = check_and_cast<Ieee80211DataFrame*>(frameToTransmit);
        MACAddress receiverAddr = dataFrameToTransmit->getReceiverAddress();
        Tid tid = dataFrameToTransmit->getTid();
        OriginatorBlockAckAgreement *agreement = context->getBlockAckAgreementHandler()->getAgreement(receiverAddr, tid);
        if (agreement == nullptr) // agreement does not exist
            return BaPolicyAction::SEND_ADDBA_REQUEST;
        return getAckPolicy(dataFrameToTransmit, agreement);
    }
}

BaPolicyAction OriginatorBlockAckPolicy::getAckPolicy(Ieee80211DataFrame* frame, OriginatorBlockAckAgreement *agreement)
{
    if (agreement->getIsAddbaResponseReceived()) {
        if (agreement->getBufferSize() == agreement->getNumSentBaPolicyFrames()) // buffer is full
            return BaPolicyAction::SEND_WITH_NORMAL_ACK;
        else if (isEligibleFrame(frame, agreement)) // checks agreement policy
            return BaPolicyAction::SEND_WITH_BLOCK_ACK;
        else
            return BaPolicyAction::SEND_WITH_NORMAL_ACK;
    }
    else
        return BaPolicyAction::SEND_WITH_NORMAL_ACK;
}

bool OriginatorBlockAckPolicy::isEligibleFrame(Ieee80211DataFrame* frame, OriginatorBlockAckAgreement *agreement)
{
    bool aMsduOk = agreement->getIsAMsduSupported() || !frame->getAMsduPresent();
    // TODO: bool baPolicy = agreement->getIsDelayedBlockAckPolicySupported() || !frame->getAckPolicy();
    return aMsduOk; // TODO: && baPolicy
}

} /* namespace ieee80211 */
} /* namespace inet */
