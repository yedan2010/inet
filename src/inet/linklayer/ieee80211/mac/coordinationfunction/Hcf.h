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
#include "inet/linklayer/ieee80211/mac/contract/ICoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/IRecipientMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/ITx.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceHandler.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/lifetime/EdcaTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorQoSMacDataService.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/RtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/TxopProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/AckProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/BlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/CtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientQoSMacDataService.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Hybrid Coordination Function.
 */
class INET_API Hcf : public ICoordinationFunction, public IFrameSequenceHandler::ICallback, public IChannelAccess::ICallback, public ITx::ICallback, public cSimpleModule
{
    protected:
        //Ieee80211Mac *mac = nullptr;

        // Transmission and Reception
        IRx *rx = nullptr;
        ITx *tx = nullptr;

        // Channel Access Methods
        Edca *edca = nullptr;
        Hcca *hcca = nullptr;

        // MAC Data Service
        OriginatorQoSMacDataService *originatorDataService = nullptr;
        RecipientQoSMacDataService *recipientDataService = nullptr;

        // MAC Procedures
        AckProcedure *recipientAckProcedure = nullptr;
        OriginatorAckProcedure *originatorAckProcedure = nullptr;
        RtsProcedure *rtsProcedure = nullptr;
        CtsProcedure *ctsProcedure = nullptr;
        OriginatorBlockAckProcedure *originatorBlockAckProcedure = nullptr;
        BlockAckProcedure *recipientBlockAckProcedure = nullptr;
        EdcaTransmitLifetimeHandler *lifetimeHandler = nullptr;
        std::vector<RecoveryProcedure *> recoveryProcedures;

        // Block Ack Agreement Handlers
        OriginatorBlockAckAgreementHandler *originatorBlockAckAgreementHandler = nullptr;
        RecipientBlockAckAgreementHandler *recipientBlockAckAgreementHandler = nullptr;

        // Ack handler
        std::vector<AckHandler *> ackHandlers;

        // Tx Opportunity
        std::vector<TxOpProcedure*> txops;

        // Queues
        std::vector<PendingQueue *> pendingQueues;
        std::vector<InProgressFrames *> inProgressFrames;

        // Frame sequence handlers
        IFrameSequenceHandler *frameSequenceHandler = nullptr;

        simtime_t sifs;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        void startFrameSequence(AccessCategory ac);
        void handleInternalCollision(std::vector<Edcaf*> internallyCollidedEdcafs);

        void sendUp(const std::vector<Ieee80211Frame*>& completeFrames);

        virtual void recipientProcessReceivedFrame(Ieee80211Frame *frame);
        virtual void recipientProcessControlFrame(Ieee80211Frame *frame);
        virtual void recipientProcessManagementFrame(Ieee80211ManagementFrame *frame);
        virtual void originatorProcessRtsProtectionFailed(Ieee80211DataOrMgmtFrame *protectedFrame) override;
        virtual void originatorProcessTransmittedFrame(Ieee80211Frame* transmittedFrame) override;
        virtual void originatorProcessReceivedFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame) override;
        virtual void originatorProcessFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame) override;
        virtual void frameSequenceFinished() override;

    public:
        // ICoordinationFunction
        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void processLowerFrame(Ieee80211Frame *frame) override;

        // IChannelAccess::ICallback
        virtual void channelAccessGranted() override;

        // IFrameSequenceHandler::ICallback
        virtual void transmitFrame(Ieee80211Frame *frame, simtime_t ifs) override;

        virtual bool isReceptionInProgress() override;
        virtual bool hasFrameToTransmit() override;

        // ITx::ICallback
        virtual void transmissionComplete() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_HCF_H
