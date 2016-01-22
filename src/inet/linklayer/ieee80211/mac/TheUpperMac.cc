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

#include "inet/common/INETUtils.h"
#include "inet/linklayer/ieee80211/mac/TheUpperMac.h"

namespace inet {
namespace ieee80211 {

Define_Module(TheUpperMac);

// TODO: copy
inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}
inline std::string suffix(const char *s, int i) {std::stringstream ss; ss << s << i; return ss.str();}

Responder::Responder(IMacParameters* params, MacUtils* utils, ITx* tx) :
     params(params),
     utils(utils),
     tx(tx)
{
}

void Responder::lowerFrameReceived(Ieee80211Frame* frame)
{
    if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
        // TODO: don't send CTS if NAV is set
        tx->transmitFrame(utils->buildCtsFrame(rtsFrame), params->getSifsTime());
    }
    else if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(frame)) {
        if (!utils->isBroadcastOrMulticast(dataFrame) && dataFrame->getAckPolicy() == NORMAL_ACK)
            tx->transmitFrame(utils->buildAckFrame(dataFrame), params->getSifsTime());
    }
    else if (auto managementFrame = dynamic_cast<Ieee80211ManagementFrame *>(frame)) {
        if (!utils->isBroadcastOrMulticast(managementFrame))
            tx->transmitFrame(utils->buildAckFrame(managementFrame), params->getSifsTime());
    }
    else if (auto blockAckReqFrame = dynamic_cast<Ieee80211BlockAckReq *>(frame)) {
        tx->transmitFrame(utils->buildBlockAckFrame(blockAckReqFrame), params->getSifsTime());
    }
    else
        throw cRuntimeError("Unknown frame");
}

void Responder::transmissionComplete()
{
    // void
}

void TheUpperMac::initialize()
{
    duplicateDetector = new QoSDuplicateDetector(); // TODO: and/or new NonQoSDuplicateDetector();
    mac = check_and_cast<Ieee80211Mac *>(getModuleByPath(par("macModule")));
    reassembly = check_and_cast<IReassembly *>(inet::utils::createOne(par("reassemblyClass")));
    rateControl = dynamic_cast<IRateControl *>(getModuleByPath(par("rateControlModule"))); // optional module
    rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
    rateSelection->setRateControl(rateControl);
    params = extractParameters(rateSelection->getSlowestMandatoryMode());
    utils = new MacUtils(params, rateSelection);
    statistics = check_and_cast<IStatistics*>(getModuleByPath(par("statisticsModule")));
    statistics->setMacUtils(utils);
    statistics->setRateControl(rateControl);
    rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
    tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
    dcf = check_and_cast<Dcf *>(getSubmodule("dcf"));
    dcf->setParamsAndUtils(params, utils);
    pcf = new Pcf();
    hcf = new Hcf(params, utils, tx, this);
    mcf = new Mcf();
    responder = new Responder(params, utils, tx);
}

IMacParameters *TheUpperMac::extractParameters(const IIeee80211Mode *slowestMandatoryMode)
{
    const IIeee80211Mode *referenceMode = slowestMandatoryMode;  // or any other; slotTime etc must be the same for all modes we use
    MacParameters *params = new MacParameters();
    params->setAddress(mac->getAddress());
    params->setShortRetryLimit(fallback(par("shortRetryLimit"), 7));
    params->setLongRetryLimit(fallback(par("longRetryLimit"), 4));
    params->setRtsThreshold(par("rtsThreshold"));
    params->setPhyRxStartDelay(referenceMode->getPhyRxStartDelay());
    params->setUseFullAckTimeout(par("useFullAckTimeout"));
    params->setSlotTime(fallback(par("slotTime"), referenceMode->getSlotTime()));
    params->setSifsTime(fallback(par("sifsTime"), referenceMode->getSifsTime()));
    return params;
}

bool TheUpperMac::sendUpIfNecessary(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame)
{
    if (duplicateDetector->isDuplicate(dataOrMgmtFrame)) {
        EV_INFO << "Duplicate frame " << dataOrMgmtFrame->getName() << ", dropping\n";
        return false;
    }
    else {
        Ieee80211DataFrame *dataFrame = dynamic_cast<Ieee80211DataFrame*>(dataOrMgmtFrame);
        if (dataFrame && dataFrame->getAMsduPresent())
            sendUpAggregatedFrame(dataFrame);
        else if (!utils->isFragment(dataOrMgmtFrame))
            mac->sendUp(dataOrMgmtFrame);
        else {
            Ieee80211DataOrMgmtFrame *completeFrame = reassembly->addFragment(dataOrMgmtFrame);
            if (completeFrame)
                mac->sendUp(completeFrame);
            else
                return false;
        }
        return true;
    }
}

void TheUpperMac::sendUpAggregatedFrame(Ieee80211DataFrame* dataFrame)
{
    EV_INFO << "MSDU aggregated frame received. Exploding it...\n";
    auto frames = msduAggregator->explodeAggregateFrame(dataFrame);
    EV_INFO << "It contained the following subframes:\n";
    for (Ieee80211DataFrame *frame : frames) {
        EV_INFO << frame << "\n";
        mac->sendUp(frame);
    }
}

void TheUpperMac::upperFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    take(frame);
    EV_INFO << "Frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << "\n";
    ASSERT(!frame->getReceiverAddress().isUnspecified());
    frame->setTransmitterAddress(params->getAddress());
    if (frame->getType() == ST_DATA)
        dcf->upperFrameReceived(frame);
    else if (frame->getType() == ST_DATA_WITH_QOS)
        hcf->upperFrameReceived(frame);
    else
        throw cRuntimeError("Unknown frame type");
}

void TheUpperMac::lowerFrameReceived(Ieee80211Frame* frame)
{
    Enter_Method("lowerFrameReceived(\"%s\")", frame->getName());
    delete frame->removeControlInfo(); // TODO
    take(frame);
    if (!utils->isForUs(frame)) {
        EV_INFO << "This frame is not for us\n";
        delete frame;
        corruptedOrNotForUsFrameReceived();
    }
    else if (dcf->isSequenceRunning())
        dcf->lowerFrameReceived(frame);
    else if (hcf->isSequenceRunning())
        hcf->lowerFrameReceived(frame);
    else {
        if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame))
            sendUpIfNecessary(dataOrMgmtFrame);
        responder->lowerFrameReceived(frame);
    }
}

void TheUpperMac::corruptedOrNotForUsFrameReceived()
{
    // void
}

void TheUpperMac::transmissionComplete()
{
    if (dcf->isSequenceRunning())
        dcf->transmissionComplete();
    else if (hcf->isSequenceRunning())
        hcf->transmissionComplete();
    else
        responder->transmissionComplete();
}

} // namespace ieee80211
} // namespace inet

