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
#include "inet/linklayer/ieee80211/mac/aggregation/MsduDeaggregation.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/QosDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/UpperMac.h"

namespace inet {
namespace ieee80211 {

Define_Module(UpperMac);

void UpperMac::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        mac = check_and_cast<Ieee80211Mac *>(getModuleByPath(par("macModule")));
        rateControl = dynamic_cast<IRateControl *>(getModuleByPath(par("rateControlModule"))); // optional module
        rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        recipientMpduHandler = check_and_cast<RecipientMpduHandler *>(getSubmodule("dcf")->getSubmodule("recipientMpduHandler"));
        recipientQosMpduHandler = check_and_cast<RecipientQoSMpduHandler *>(getSubmodule("edca")->getSubmodule("recipientQoSMpduHandler"));
        rateSelection->setRateControl(rateControl);
        rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        dcf = check_and_cast<Dcf *>(getSubmodule("dcf"));
        //pcf = new Pcf();
        hcf = new Hcf(check_and_cast<Edca*>(getSubmodule("edca")), nullptr); // TODO: nullptr -> check_and_cast<Hcca*>(getSubmodule("Hcca"))
        //mcf = new Mcf();
    }
}

bool UpperMac::isForUs(Ieee80211Frame *frame) const
{
    return frame->getReceiverAddress() == mac->getAddress() || (frame->getReceiverAddress().isMulticast() && !isSentByUs(frame));
}

bool UpperMac::isSentByUs(Ieee80211Frame *frame) const
{
    if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame))
        return dataOrMgmtFrame->getAddress3() == mac->getAddress();
    else
        return false;
}

void UpperMac::upperFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    take(frame);
    EV_INFO << "Frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << "\n";
    ASSERT(!frame->getReceiverAddress().isUnspecified());
    if (frame->getType() == ST_DATA)
        dcf->upperFrameReceived(frame);
    else if (frame->getType() == ST_DATA_WITH_QOS)
        hcf->upperFrameReceived(frame);
    else
        throw cRuntimeError("Unknown frame type");
}

bool UpperMac::isQoSFrame(Ieee80211Frame* frame)
{
    bool categoryThreeActionFrame = false;
    if (auto actionFrame = dynamic_cast<Ieee80211ActionFrame*>(frame)) {
        categoryThreeActionFrame = actionFrame->getCategory() == 3;
    }
    return frame->getType() == ST_DATA_WITH_QOS || frame->getType() == ST_BLOCKACK ||
           frame->getType() == ST_BLOCKACK_REQ || categoryThreeActionFrame;
}

void UpperMac::lowerFrameReceived(Ieee80211Frame* frame)
{
    Enter_Method("lowerFrameReceived(\"%s\")", frame->getName());
    delete frame->removeControlInfo(); // TODO
    take(frame);
    if (!isForUs(frame)) {
        EV_INFO << "This frame is not for us\n";
        delete frame;
    }
    // TODO: collision controller
    else if (dcf->isSequenceRunning())
        dcf->lowerFrameReceived(frame);
    else if (hcf->isSequenceRunning())
        hcf->lowerFrameReceived(frame);
    else if (isQoSFrame(frame))
        recipientQosMpduHandler->processReceivedFrame(frame);
    else
        recipientMpduHandler->processReceivedFrame(frame);
}

} // namespace ieee80211
} // namespace inet
