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

#include "AckProcedure.h"

namespace inet {
namespace ieee80211 {

AckProcedure::AckProcedure(IRateSelection *rateSelection) :
    rateSelection(rateSelection)
{
   this->sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
   this->slotTime = rateSelection->getSlowestMandatoryMode()->getSlotTime();
   this->phyRxStartDelay = rateSelection->getSlowestMandatoryMode()->getPhyRxStartDelay();
}

void AckProcedure::processReceivedFrame(Ieee80211Frame* frame)
{
}

void AckProcedure::processTransmittedAck(Ieee80211ACKFrame* ack)
{
}

simtime_t AckProcedure::getAckEarlyTimeout() const
{
    // Note: This excludes ACK duration. If there's no RXStart indication within this interval, retransmission should begin immediately
    return sifs + slotTime + phyRxStartDelay;
}


simtime_t AckProcedure::getAckDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
}

simtime_t AckProcedure::getAckFullTimeout() const
{
    return sifs + slotTime + getAckDuration();
}


Ieee80211ACKFrame* AckProcedure::buildAck(Ieee80211Frame* frame) {
    // TODO: broadcast checking
    // TODO: we don't support addba requests needing ACK.
    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame*>(frame))
        if (dataFrame->getAckPolicy() != NORMAL_ACK)
            return nullptr;

    if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame*>(frame)) {
        Ieee80211ACKFrame *ack = new Ieee80211ACKFrame("ACK");
        ack->setReceiverAddress(dataOrMgmtFrame->getTransmitterAddress());
        if (!frame->getMoreFragments())
            ack->setDuration(0);
        else
            ack->setDuration(frame->getDuration() - sifs - rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK));
        // setFrameMode(ack, rateSelection->getModeForControlFrame(ack)); // FIXME: move
        return ack;
    }

    return nullptr;
}

} /* namespace ieee80211 */
} /* namespace inet */

