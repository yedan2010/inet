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

#ifndef __INET_UPPERMAC_H
#define __INET_UPPERMAC_H

#include "inet/linklayer/ieee80211/mac/contract/IRateControl.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/contract/IRx.h"
#include "inet/linklayer/ieee80211/mac/contract/IUpperMac.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Dcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Mcf.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/Pcf.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientQoSMpduHandler.h"

namespace inet {
namespace ieee80211 {

class INET_API UpperMac : public cSimpleModule, public IUpperMac
{
    protected:
        Ieee80211Mac *mac = nullptr; // TODO: remove
        IRateControl *rateControl = nullptr;
        IRateSelection *rateSelection = nullptr;

        IRx *rx = nullptr;
        ITx *tx = nullptr;

        Dcf *dcf = nullptr;
        Pcf *pcf = nullptr;
        Hcf *hcf = nullptr;
        Mcf *mcf = nullptr;

        RecipientQoSMpduHandler *recipientQosMpduHandler = nullptr;
        RecipientMpduHandler *recipientMpduHandler = nullptr;

    protected:
        virtual int numInitStages() const override {return NUM_INIT_STAGES;}
        virtual void initialize(int stage) override;

        // TODO: From utils:
        bool isForUs(Ieee80211Frame *frame) const;
        bool isSentByUs(Ieee80211Frame *frame) const;

        bool isQoSFrame(Ieee80211Frame *frame);

    public:
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void lowerFrameReceived(Ieee80211Frame *frame) override;
};

} // namespace ieee80211
} // namespace inet

#endif
