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

#ifndef __INET_THEUPPERMAC_H
#define __INET_THEUPPERMAC_H

#include "inet/linklayer/ieee80211/mac/CoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/DuplicateDetectors.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/linklayer/ieee80211/mac/IFragmentation.h"
#include "inet/linklayer/ieee80211/mac/IMacParameters.h"
#include "inet/linklayer/ieee80211/mac/IMsduAggregation.h"
#include "inet/linklayer/ieee80211/mac/IRateControl.h"
#include "inet/linklayer/ieee80211/mac/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/IRx.h"
#include "inet/linklayer/ieee80211/mac/IStatistics.h"
#include "inet/linklayer/ieee80211/mac/IUpperMac.h"

namespace inet {
namespace ieee80211 {

class Responder
{
    protected:
        IMacParameters *params = nullptr;
        MacUtils *utils = nullptr;
        ITx *tx = nullptr;

    public:
        Responder(IMacParameters *params, MacUtils *utils, ITx *tx);

        virtual void lowerFrameReceived(Ieee80211Frame *frame);
        virtual void transmissionComplete();
};

class TheUpperMac : public cSimpleModule, public IUpperMac
{
    protected:
        IMacParameters *params = nullptr;
        MacUtils *utils = nullptr;

        IDuplicateDetector *duplicateDetector = nullptr;
        Ieee80211Mac *mac = nullptr;
        IMsduAggregation *msduAggregator = nullptr;
        IRateControl *rateControl = nullptr;
        IRateSelection *rateSelection = nullptr;
        IReassembly *reassembly = nullptr;
        IRx *rx = nullptr;
        IStatistics *statistics = nullptr;
        ITx *tx = nullptr;

        Dcf *dcf = nullptr;
        Pcf *pcf = nullptr;
        Hcf *hcf = nullptr;
        Mcf *mcf = nullptr;
        Responder *responder = nullptr;

    protected:
        virtual void initialize() override;
        virtual IMacParameters *extractParameters(const IIeee80211Mode *slowestMandatoryMode);

        virtual bool sendUpIfNecessary(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame);
        virtual void sendUpAggregatedFrame(Ieee80211DataFrame* dataFrame);

    public:
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void lowerFrameReceived(Ieee80211Frame *frame) override;
        virtual void corruptedOrNotForUsFrameReceived() override;
        virtual void transmissionComplete() override;
};

} // namespace ieee80211
} // namespace inet

#endif
