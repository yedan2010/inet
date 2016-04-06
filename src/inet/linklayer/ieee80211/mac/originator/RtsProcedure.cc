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

#include "RtsProcedure.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"

namespace inet {
namespace ieee80211 {

RtsProcedure::RtsProcedure(IRateSelection *rateSelection) :
    rateSelection(rateSelection)
{
    this->rtsThreshold = -1; // FIXME
    // TODO: control bitrate
    this->sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
    this->slotTime = rateSelection->getSlowestMandatoryMode()->getSlotTime();
    this->phyRxStartDelay = rateSelection->getSlowestMandatoryMode()->getPhyRxStartDelay();
}

Ieee80211RTSFrame *RtsProcedure::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const
{
    const IIeee80211Mode *mode = dataFrame->getControlInfo() ? getFrameMode(dataFrame) : nullptr;
    if (isBroadcastOrMulticast(dataFrame))
        mode = rateSelection->getModeForMulticastDataOrMgmtFrame(dataFrame);
    else
        mode = rateSelection->getModeForUnicastDataOrMgmtFrame(dataFrame);
    return buildRtsFrame(dataFrame, mode);
}

Ieee80211RTSFrame *RtsProcedure::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const
{
    // protect CTS + Data + ACK
    simtime_t duration =
            3 * sifs
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS)
            + dataFrameMode->getDuration(dataFrame->getBitLength())
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
    return buildRtsFrame(dataFrame->getReceiverAddress(), duration);
}

Ieee80211RTSFrame *RtsProcedure::buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const
{
    Ieee80211RTSFrame *rts = new Ieee80211RTSFrame("RTS");
    rts->setReceiverAddress(receiverAddress);
    rts->setDuration(duration);
    setFrameMode(rts, rateSelection->getModeForControlFrame(rts));
    return rts;
}

simtime_t RtsProcedure::getCtsDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS);
}

simtime_t RtsProcedure::getCtsEarlyTimeout() const
{
    return sifs + slotTime + phyRxStartDelay; // see getAckEarlyTimeout()
}

simtime_t RtsProcedure::getCtsFullTimeout() const
{
    return sifs + slotTime + getCtsDuration();
}

bool RtsProcedure::isBroadcastOrMulticast(Ieee80211Frame *frame) const
{
    return frame && frame->getReceiverAddress().isMulticast();  // also true for broadcast frames
}

const IIeee80211Mode *RtsProcedure::getFrameMode(Ieee80211Frame *frame) const
{
    Ieee80211TransmissionRequest *ctrl = check_and_cast<Ieee80211TransmissionRequest*>(frame->getControlInfo());
    return ctrl->getMode();
}

// TODO: move this part to somewhere else
Ieee80211Frame *RtsProcedure::setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const
{
    ASSERT(frame->getControlInfo() == nullptr);
    Ieee80211TransmissionRequest *ctrl = new Ieee80211TransmissionRequest();
    ctrl->setMode(mode);
    frame->setControlInfo(ctrl);
    return frame;
}

} /* namespace ieee80211 */
} /* namespace inet */
