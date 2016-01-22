//
// Copyright (C) 2015 Andras Varga
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
// Author: Andras Varga
//

#include "MacUtils.h"
#include "IMacParameters.h"
#include "IRateSelection.h"
#include "inet/linklayer/ieee80211/oldmac/Ieee80211Consts.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211DSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HRDSSSMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211HTMode.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211OFDMMode.h"

namespace inet {
namespace ieee80211 {

MacUtils::MacUtils(IMacParameters *params, IRateSelection *rateSelection) : params(params), rateSelection(rateSelection)
{
}

simtime_t MacUtils::getAckDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
}

simtime_t MacUtils::getCtsDuration() const
{
    return rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS);
}

simtime_t MacUtils::getTimeout(simtime_t duration, bool usePifs) const
{
    return (usePifs ? timingParameters->getPifsTime() : timingParameters->getSifsTime()) + timingParameters->getSlotTime() + duration;
}

simtime_t MacUtils::getEarlyTimeout(bool usePifs) const
{
    return (usePifs ? timingParameters->getPifsTime() : timingParameters->getSifsTime()) + timingParameters->getSlotTime() + params->getPhyRxStartDelay();
}

simtime_t MacUtils::getAckEarlyTimeout() const
{
    // Note: This excludes ACK duration. If there's no RXStart indication within this interval, retransmission should begin immediately
    return timingParameters->getSifsTime() + timingParameters->getSlotTime() + params->getPhyRxStartDelay();
}

simtime_t MacUtils::getAckFullTimeout() const
{
    return timingParameters->getSifsTime() + timingParameters->getSlotTime() + getAckDuration();
}

simtime_t MacUtils::getCtsEarlyTimeout() const
{
    return timingParameters->getSifsTime() + timingParameters->getSlotTime() + params->getPhyRxStartDelay(); // see getAckEarlyTimeout()
}

simtime_t MacUtils::getCtsFullTimeout() const
{
    return timingParameters->getSifsTime() + timingParameters->getSlotTime() + getCtsDuration();
}

Ieee80211RTSFrame *MacUtils::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame) const
{
    const IIeee80211Mode *mode = dataFrame->getControlInfo() ? getFrameMode(dataFrame) : nullptr;
    if (isBroadcastOrMulticast(dataFrame))
        mode = rateSelection->getModeForMulticastDataOrMgmtFrame(dataFrame);
    else
        mode = rateSelection->getModeForUnicastDataOrMgmtFrame(dataFrame);
    return buildRtsFrame(dataFrame, mode);
}

Ieee80211RTSFrame *MacUtils::buildRtsFrame(Ieee80211DataOrMgmtFrame *dataFrame, const IIeee80211Mode *dataFrameMode) const
{
    // protect CTS + Data + ACK
    simtime_t duration =
            3 * timingParameters->getSifsTime()
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS)
            + dataFrameMode->getDuration(dataFrame->getBitLength())
            + rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK);
    return buildRtsFrame(dataFrame->getReceiverAddress(), duration);
}

Ieee80211RTSFrame *MacUtils::buildRtsFrame(const MACAddress& receiverAddress, simtime_t duration) const
{
    Ieee80211RTSFrame *rts = new Ieee80211RTSFrame("RTS");
    rts->setTransmitterAddress(params->getAddress());
    rts->setReceiverAddress(receiverAddress);
    rts->setDuration(duration);
    setFrameMode(rts, rateSelection->getModeForControlFrame(rts));
    return rts;
}

Ieee80211CTSFrame *MacUtils::buildCtsFrame(Ieee80211RTSFrame *rtsFrame) const
{
    Ieee80211CTSFrame *cts = new Ieee80211CTSFrame("CTS");
    cts->setReceiverAddress(rtsFrame->getTransmitterAddress());
    cts->setDuration(rtsFrame->getDuration() - timingParameters->getSifsTime() - rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_CTS));
    setFrameMode(rtsFrame, rateSelection->getModeForControlFrame(rtsFrame));
    return cts;
}

Ieee80211ACKFrame *MacUtils::buildAckFrame(Ieee80211DataOrMgmtFrame *dataFrame) const
{
    Ieee80211ACKFrame *ack = new Ieee80211ACKFrame("ACK");
    ack->setReceiverAddress(dataFrame->getTransmitterAddress());
    if (!dataFrame->getMoreFragments())
        ack->setDuration(0);
    else
        ack->setDuration(dataFrame->getDuration() - timingParameters->getSifsTime() - rateSelection->getResponseControlFrameMode()->getDuration(LENGTH_ACK));
    setFrameMode(ack, rateSelection->getModeForControlFrame(ack));
    return ack;
}

Ieee80211BlockAckReq* MacUtils::buildBlockAckReqFrame(const MACAddress& receiverAddress, int startingSequenceNumber) const
{
    Ieee80211CompressedBlockAckReq *blockAckReq = new Ieee80211CompressedBlockAckReq("BlockAckReq");
    blockAckReq->setTransmitterAddress(params->getAddress());
    blockAckReq->setReceiverAddress(receiverAddress);
    blockAckReq->setDuration(timingParameters->getSifsTime());
    // TODO:
    blockAckReq->setStartingSequenceNumber(startingSequenceNumber);
    return blockAckReq;
}

Ieee80211BlockAck* MacUtils::buildBlockAckFrame(Ieee80211BlockAckReq* blockAckReq) const
{
    Ieee80211CompressedBlockAck *blockAck = new Ieee80211CompressedBlockAck("BlockAck");
    blockAck->setTransmitterAddress(params->getAddress());
    blockAck->setReceiverAddress(blockAckReq->getTransmitterAddress());
    blockAck->setDuration(0);
    blockAck->setCompressedBitmap(true);
    BitVector &bitmap = blockAck->getBlockAckBitmap();
    for (int i = 0; i < 64; i++) {
        bitmap.setBit(i, true);
    }
    return blockAck;
}

Ieee80211Frame *MacUtils::setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const
{
    ASSERT(frame->getControlInfo() == nullptr);
    Ieee80211TransmissionRequest *ctrl = new Ieee80211TransmissionRequest();
    ctrl->setMode(mode);
    frame->setControlInfo(ctrl);
    return frame;
}

const IIeee80211Mode *MacUtils::getFrameMode(Ieee80211Frame *frame) const
{
    Ieee80211TransmissionRequest *ctrl = check_and_cast<Ieee80211TransmissionRequest*>(frame->getControlInfo());
    return ctrl->getMode();
}

bool MacUtils::isForUs(Ieee80211Frame *frame) const
{
    return frame->getReceiverAddress() == params->getAddress() || (frame->getReceiverAddress().isMulticast() && !isSentByUs(frame));
}

bool MacUtils::isSentByUs(Ieee80211Frame *frame) const
{
    if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame))
        return dataOrMgmtFrame->getAddress3() == params->getAddress();
    else
        return false;
}


bool MacUtils::isBroadcastOrMulticast(Ieee80211Frame *frame) const
{
    return frame && frame->getReceiverAddress().isMulticast();  // also true for broadcast frames
}

bool MacUtils::isBroadcast(Ieee80211Frame *frame) const
{
    return frame && frame->getReceiverAddress().isBroadcast();
}

bool MacUtils::isFragment(Ieee80211DataOrMgmtFrame *frame) const
{
    return frame->getFragmentNumber() != 0 || frame->getMoreFragments() == true;
}

bool MacUtils::isCts(Ieee80211Frame *frame) const
{
    return dynamic_cast<Ieee80211CTSFrame *>(frame);
}

bool MacUtils::isAck(Ieee80211Frame *frame) const
{
    return dynamic_cast<Ieee80211ACKFrame *>(frame);
}

simtime_t MacUtils::getTxopLimit(AccessCategory ac, const IIeee80211Mode *mode)
{
    switch (ac)
    {
        case AC_BK: return 0;
        case AC_BE: return 0;
        case AC_VI:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(6.016).get();
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(3.008).get();
            else return 0;
        case AC_VO:
            if (dynamic_cast<const Ieee80211DsssMode*>(mode) || dynamic_cast<const Ieee80211HrDsssMode*>(mode)) return ms(3.264).get();
            else if (dynamic_cast<const Ieee80211HTMode*>(mode) || dynamic_cast<const Ieee80211OFDMMode*>(mode)) return ms(1.504).get();
            else return 0;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return 0;
}

int MacUtils::getAifsNumber(AccessCategory ac)
{
    switch (ac)
    {
        case AC_BK: return 7;
        case AC_BE: return 3;
        case AC_VI: return 2;
        case AC_VO: return 2;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}

int MacUtils::getCwMax(AccessCategory ac, int aCwMax, int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMax;
        case AC_BE: return aCwMax;
        case AC_VI: return aCwMin;
        case AC_VO: return (aCwMin + 1) / 2 - 1;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}

int MacUtils::getCwMin(AccessCategory ac, int aCwMin)
{
    switch (ac)
    {
        case AC_BK: return aCwMin;
        case AC_BE: return aCwMin;
        case AC_VI: return (aCwMin + 1) / 2 - 1;
        case AC_VO: return (aCwMin + 1) / 4 - 1;
        case AC_NUMCATEGORIES: break;
    }
    throw cRuntimeError("Unknown access category = %d", ac);
    return -1;
}

} // namespace ieee80211
} // namespace inet

