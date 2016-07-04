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

#include "inet/common/ModuleAccess.h"
#include "inet/common/NotifierConsts.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateControl.h"
#include "inet/linklayer/ieee80211/mac/rateselection/BasicRateSelection.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"
#include "inet/physicallayer/ieee80211/mode/IIeee80211Mode.h"

namespace inet {
namespace ieee80211 {

Define_Module(BasicRateSelection);

void BasicRateSelection::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        getContainingNicModule(this)->subscribe(NF_MODESET_CHANGED, this);
    }
    else if (stage == INITSTAGE_LINK_LAYER_2) {
        double controlBitrate = par("controlBitrate");
        controlFrameMode = (controlBitrate == -1) ? modeSet->getSlowestMandatoryMode() : modeSet->getMode(bps(controlBitrate));

        double multicastBitrate = par("multicastBitrate");
        multicastFrameMode = (multicastBitrate == -1) ? modeSet->getFastestMandatoryMode() : modeSet->getMode(bps(multicastBitrate));

        double dataBitrate = par("dataBitrate");
        dataFrameMode = (dataBitrate == -1) ? modeSet->getFastestMode() : modeSet->getMode(bps(dataBitrate));

        slowestMandatoryMode = modeSet->getSlowestMandatoryMode();
    }
}

void BasicRateSelection::setRateControl(IRateControl *rateControl)
{
    this->rateControl = rateControl;
    if (rateControl)
        rateControl->initialize(modeSet, dataFrameMode);
}

const IIeee80211Mode *BasicRateSelection::getSlowestMandatoryMode()
{
    return slowestMandatoryMode;
}

const IIeee80211Mode *BasicRateSelection::getModeForUnicastDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame)
{
    return rateControl ? rateControl->getRate() : dataFrameMode;
}

const IIeee80211Mode *BasicRateSelection::getModeForMulticastDataOrMgmtFrame(Ieee80211DataOrMgmtFrame *frame)
{
    return multicastFrameMode;
}

const IIeee80211Mode *BasicRateSelection::getModeForControlFrame(Ieee80211Frame *controlFrame)
{
    return controlFrameMode;
}

const IIeee80211Mode *BasicRateSelection::getResponseControlFrameMode()
{
    return controlFrameMode;
}

void BasicRateSelection::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    Enter_Method("receiveModeSetChangeNotification");
    if (signalID == NF_MODESET_CHANGED)
        modeSet = check_and_cast<Ieee80211ModeSet*>(obj);
}

} // namespace ieee80211
} // namespace inet
