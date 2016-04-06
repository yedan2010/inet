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

#include "OriginatorAckProcedure.h"

namespace inet {
namespace ieee80211 {

OriginatorAckProcedure::OriginatorAckProcedure(IRateSelection *rateSelection) :
    rateSelection(rateSelection)
{
   this->sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
   this->slotTime = rateSelection->getSlowestMandatoryMode()->getSlotTime();
   this->phyRxStartDelay = rateSelection->getSlowestMandatoryMode()->getPhyRxStartDelay();
}

simtime_t OriginatorAckProcedure::getAckEarlyTimeout() const
{
    // Note: This excludes ACK duration. If there's no RXStart indication within this interval, retransmission should begin immediately
    return sifs + slotTime + phyRxStartDelay;
}

simtime_t OriginatorAckProcedure::getAckDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
}

simtime_t OriginatorAckProcedure::getAckFullTimeout() const
{
    return sifs + slotTime + getAckDuration();
}


} /* namespace ieee80211 */
} /* namespace inet */
