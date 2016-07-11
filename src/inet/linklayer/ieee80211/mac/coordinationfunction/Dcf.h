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

#ifndef __INET_DCF_H
#define __INET_DCF_H

#include "inet/linklayer/ieee80211/mac/contract/ICoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/contract/IFrameSequenceHandler.h"
#include "inet/linklayer/ieee80211/mac/channelaccess/DcfChannelAccess.h"

#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/lifetime/DcfTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorMacDataService.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/RtsProcedure.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"

#include "inet/linklayer/ieee80211/mac/originator/OriginatorMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientMpduHandler.h"


namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Distributed Coordination Function.
 */
class INET_API Dcf : public ICoordinationFunction, public IFrameSequenceHandler::ICallback, public IChannelAccess::ICallback, public ITx::ICallback, public cSimpleModule
{
    protected:
        IRx *rx = nullptr;
        ITx *tx = nullptr;

        FrameSequenceContext *context = nullptr;
        OriginatorMpduHandler *originatorMpduHandler = nullptr;
        RecipientMpduHandler *recipientMpduHandler = nullptr;
        PendingQueue *pendingQueue = nullptr;
        OriginatorMacDataService *dataService = nullptr;
        InProgressFrames *inProgressFrames = nullptr;
        OriginatorAckProcedure *ackProcedure = nullptr;
        AckHandler *ackHandler = nullptr;
        DcfTransmitLifetimeHandler *lifetimeHandler = nullptr;
        RecoveryProcedure *recoveryProcedure = nullptr;
        RtsProcedure *rtsProcedure = nullptr;

        IFrameSequenceHandler *frameSequenceHandler = nullptr;

        DcfChannelAccess *dcfChannelAccess = nullptr;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

    public:
        // ICoordinationFunction
        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame) override;
        virtual void processLowerFrame(Ieee80211Frame *frame) override;

        // IContentionBasedChannelAccess::ICallback
        virtual void channelAccessGranted() override;

        // IFrameSequenceHandler::ICallback
        virtual void transmitFrame(Ieee80211Frame *frame, simtime_t ifs) override;

        virtual bool isReceptionInProgress() override;
        virtual bool hasFrameToTransmit() override;

        virtual void processRtsProtectionFailed(Ieee80211DataOrMgmtFrame *protectedFrame) override;
        virtual void processTransmittedFrame(Ieee80211Frame* transmittedFrame) override;
        virtual void processReceivedFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame) override;
        virtual void processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame) override;
        virtual void frameSequenceFinished() override;

        // ITx::ICallback
        virtual void transmissionComplete() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_DCF_H
