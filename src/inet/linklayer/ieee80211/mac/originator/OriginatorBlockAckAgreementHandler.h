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

#ifndef __INET_ORIGINATORBLOCKACKAGREEMENTHANDLER_H
#define __INET_ORIGINATORBLOCKACKAGREEMENTHANDLER_H

#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ieee80211/mac/blockack/OriginatorBlockAckAgreement.h"
#include "inet/linklayer/ieee80211/mac/common/Ieee80211Defs.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

class INET_API OriginatorBlockAckAgreementHandler : public cSimpleModule
{
    // FIXME: timeout
    protected:
        IRateSelection *rateSelection = nullptr;
        simtime_t sifs = -1;
        simtime_t slotTime = -1;
        simtime_t phyRxStartDelay = -1;

        int maximumAllowedBufferSize = -1;
        bool isAMsduSupported = false;
        bool isDelayedBlockAckPolicySupported = false;
        simtime_t blockAckTimeoutValue = -1;

        std::map<std::pair<MACAddress, Tid>, OriginatorBlockAckAgreement *> blockAckAgreements;

    protected:
        virtual Ieee80211Delba *buildDelba(MACAddress receiverAddr, Tid tid, int reasonCode);

    public:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *msg) override;

        OriginatorBlockAckAgreement *getAgreement(MACAddress receiverAddr, Tid tid);

        virtual void processReceivedDelba(Ieee80211Delba* delba);
        virtual void processAddbaRequest(Ieee80211AddbaRequest *addbaRequest);
        virtual void processTransmittedDelbaRequest(Ieee80211Delba *frame);
        virtual void processReceivedAddbaResponse(Ieee80211AddbaResponse *addbaResponse);
        virtual Ieee80211AddbaRequest *buildAddbaRequest(MACAddress receiverAddr, Tid tid, int startingSequenceNumber);

        simtime_t getAddbaRequestDuration(Ieee80211AddbaRequest *addbaReq) const;
        simtime_t getAddbaRequestEarlyTimeout() const;

        Ieee80211Frame *setFrameMode(Ieee80211Frame *frame, const IIeee80211Mode *mode) const;

};

} // namespace ieee80211
} // namespace inet

#endif
