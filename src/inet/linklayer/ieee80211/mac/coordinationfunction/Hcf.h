//
// Copyright (C) 2016 OpenSim Ltd.
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

#ifndef __INET_HCF_H
#define __INET_HCF_H

#include "inet/linklayer/ieee80211/mac/channelaccess/Edca.h"
#include "inet/linklayer/ieee80211/mac/channelaccess/Hcca.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Hybrid Coordination Function.
 */
class INET_API Hcf : public ICoordinationFunction, public IFrameSequenceHandler::ICallback, public IContentionBasedChannelAccess::ICallback, public ITx::ICallback, public cSimpleModule
{
    protected:
        Edca *edca = nullptr;
        OriginatorQoSMpduHandler *originatorMpduHandler = nullptr;
        RecipientQoSMpduHandler *recipientMpduHandler = nullptr;

        std::vector<PendingQueue *> pendingQueues;
        std::vector<InProgressFrames *> inprogressFrames;
        std::vector<AckHandler *> ackHandlers;
        std::vector<RecoveryProcedure *> recoveryProcedures;

        OriginatorQoSMacDataService *macDataService = nullptr;
        OriginatorAckProcedure *ackProcedure = nullptr;
        RtsProcedure *rtsProcedure = nullptr;
        TxOpProcedure *txopProcedure = nullptr;
        OriginatorBlockAckAgreementHandler *originatorBlockAckAgreementHandler = nullptr;
        RecipientBlockAckAgreementHandler *recipientBlockAckAgreementHandler = nullptr;
        OriginatorBlockAckProcedure *blockAckProcedure = nullptr;
        EdcaTransmitLifetimeHandler *lifetimeHandler = nullptr;

        const Ieee80211ModeSet *modeSet = nullptr;

        Hcca *hcca = nullptr; // TODO: unimplemented,

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

    public:
        virtual bool isSequenceRunning() { return edca->isSequenceRunning(); }

        // ICoordinationFunction
        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void processLowerFrame(Ieee80211Frame *frame) override;

        // IContentionBasedChannelAccess::ICallback
        virtual void channelAccessGranted() override;

        // IFrameSequenceHandler::ICallback
        virtual void transmitFrame(Ieee80211Frame *frame, simtime_t ifs) override;
        virtual void frameSequenceFinished() override;
        virtual bool isReceptionInProgress() override;

        // ITx::ICallback
        virtual bool transmissionComplete() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_HCF_H
