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

#ifndef __INET_QOSORIGINATORMPDUHANDLER_H
#define __INET_QOSORIGINATORMPDUHANDLER_H

#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/lifetime/EdcaTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorQoSMacDataService.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/RtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/TxopProcedure.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"


namespace inet {
namespace ieee80211 {

class OriginatorQoSMpduHandler : public IOriginatorMpduHandler, public cSimpleModule
{
    protected:
        PendingQueue *pendingQueue = nullptr;
        InProgressFrames *inProgressFrames = nullptr;
        OriginatorQoSMacDataService *dataService = nullptr;
        AckHandler *ackHandler = nullptr;
        OriginatorAckProcedure *ackProcedure = nullptr;
        OriginatorBlockAckProcedure *blockAckProcedure = nullptr;
        OriginatorBlockAckAgreementHandler *originatorBlockAckAgreementHandler = nullptr;
        RecipientBlockAckAgreementHandler *recipientBlockAckAgreementHandler = nullptr;
        RecoveryProcedure *recoveryProcedure = nullptr;
        EdcaTransmitLifetimeHandler *lifetimeHandler = nullptr;
        RtsProcedure *rtsProcedure = nullptr;
        TxOpProcedure *txopProcedure = nullptr;

        const Ieee80211ModeSet *modeSet = nullptr;

        AccessCategory ac = AccessCategory(-1);

    protected:
        s getTxopLimit(const IIeee80211Mode *mode);

    public:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        virtual void processRtsProtectionFailed(Ieee80211DataOrMgmtFrame *protectedFrame) override;
        virtual void processTransmittedFrame(Ieee80211Frame* transmittedFrame) override;
        virtual void processReceivedFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame) override;
        virtual void processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame) override;
        virtual void processBlockAckAgreementTerminated(OriginatorBlockAckAgreement *agreement);

        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame* frame) override;
        virtual bool hasFrameToTransmit() override;

        // FIXME: Edca
        virtual bool internalCollision();

        virtual FrameSequenceContext* buildContext() override;

        // TODO: kludge
        virtual int getCw() override { return recoveryProcedure->getCw(); }
        AccessCategory getAccessCategory(const char *ac);
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_QOSORIGINATORMPDUHANDLER_H
