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

#ifndef __INET_FRAMESEQUENCECONTEXT_H
#define __INET_FRAMESEQUENCECONTEXT_H

#include "inet/linklayer/ieee80211/mac/common/MacParameters.h"
#include "inet/linklayer/ieee80211/mac/common/TimingParameters.h"
#include "inet/linklayer/ieee80211/mac/contract/IFrameSequence.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorBlockAckProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/RtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/originator/TxopProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/BlockAckProcedure.h"
#include "inet/physicallayer/ieee80211/mode/Ieee80211ModeSet.h"

namespace inet {
namespace ieee80211 {

class INET_API FrameSequenceContext
{
    protected:
        PendingQueue *pendingQueue = nullptr;
        InProgressFrames *inProgressFrames = nullptr;
        OriginatorAckProcedure *ackProcedure = nullptr;
        RtsProcedure *rtsProcedure = nullptr;
        TxOpProcedure *txopProcedure = nullptr;
        OriginatorBlockAckProcedure *blockAckProcedure = nullptr;
        OriginatorBlockAckAgreementHandler *blockAckAgreementHandler = nullptr;

        const Ieee80211ModeSet *modeSet = nullptr;

        std::vector<IFrameSequenceStep *> steps;

    public:
        FrameSequenceContext(PendingQueue *pendingQueue, InProgressFrames *inProgressFrames, OriginatorAckProcedure *ackProcedure, RtsProcedure *rtsProcedure, TxOpProcedure *txopProcedure, OriginatorBlockAckProcedure *blockAckProcedure, OriginatorBlockAckAgreementHandler *blockAckAgreementHandler, const Ieee80211ModeSet *modeSet);
        virtual ~FrameSequenceContext();

        virtual void insertPendingFrame(Ieee80211DataOrMgmtFrame *pendingFrame) { pendingQueue->insert(pendingFrame); }

        virtual void addStep(IFrameSequenceStep *step) { steps.push_back(step); }
        virtual int getNumSteps() const { return steps.size(); }
        virtual IFrameSequenceStep *getStep(int i) const { return steps[i]; }
        virtual IFrameSequenceStep *getLastStep() const { return steps.size() > 0 ? steps.back() : nullptr; }
        virtual IFrameSequenceStep *getStepBeforeLast() const { return steps.size() > 1 ? steps[steps.size() - 2] : nullptr; }

        virtual std::vector<Ieee80211DataFrame*> getOutstandingFrames();
        virtual std::map<MACAddress, std::vector<Ieee80211DataFrame*>> getOutstandingFramesPerReceiver();

        OriginatorAckProcedure* getAckProcedure() { return ackProcedure; }
        InProgressFrames* getInProgressFrames() { return inProgressFrames; }
        RtsProcedure* getRtsProcedure() { return rtsProcedure; }
        TxOpProcedure* getTxopProcedure() { return txopProcedure; }
        OriginatorBlockAckProcedure* getBlockAckProcedure() { return blockAckProcedure; }
        OriginatorBlockAckAgreementHandler* getBlockAckAgreementHandler() { return blockAckAgreementHandler; }
        const Ieee80211ModeSet* getModeSet() { return modeSet; }

        // TODO: pifs
};

} // namespace ieee80211
} // namespace inet

#endif
