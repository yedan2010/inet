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

#include "inet/linklayer/ieee80211/mac/FrameExtractor.h"
#include "inet/linklayer/ieee80211/mac/IFrameSequence.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/MacParameters.h"
#include "inet/linklayer/ieee80211/mac/MacUtils.h"
#include "inet/linklayer/ieee80211/mac/TimingParameters.h"

namespace inet {
namespace ieee80211 {

class INET_API TxOp
{
    public:
        enum class Type {
            EDCA,
            HCCA,
        };

    protected:
        Type type;
        simtime_t start;
        simtime_t limit;

    public:
        TxOp(Type type, simtime_t limit) : type(type), start(simTime()), limit(limit)
        { }

        virtual Type getType() const { return type; }
        virtual simtime_t getStart() const { return start; }
        virtual simtime_t getLimit() const { return limit; }
        virtual simtime_t getRemaining() const { auto now = simTime(); return now > start + limit ? 0 : now - start; }
};

class INET_API FrameSequenceContext {
    protected:
        InProgressFrames *inProgressFrames = nullptr;
        TxOp *txOp = nullptr;
        TimingParameters *timingParameters = nullptr;
        std::vector<IFrameSequenceStep *> steps;

    public:
        MacUtils *utils = nullptr;
        IMacParameters *params = nullptr;
        FrameExtractor *extractor = nullptr;

    public:
        FrameSequenceContext(InProgressFrames *inProgressFrames, MacUtils *utils, IMacParameters *params, TimingParameters *timingParameters, TxOp *txOp);
        virtual ~FrameSequenceContext();

        virtual void addStep(IFrameSequenceStep *step) { steps.push_back(step); }
        virtual int getNumSteps() const { return steps.size(); }
        virtual IFrameSequenceStep *getStep(int i) const { return steps[i]; }
        virtual IFrameSequenceStep *getLastStep() const { return steps.size() > 0 ? steps.back() : nullptr; }
        virtual IFrameSequenceStep *getStepBeforeLast() const { return steps.size() > 1 ? steps[steps.size() - 2] : nullptr; }

        virtual bool hasTxOp() const { return txOp != nullptr; }
        virtual const TxOp *getTxOp() const { return txOp; }

        virtual bool hasFrameToTransmit();
        virtual Ieee80211Frame *getFrameToTransmit();
};

} // namespace ieee80211
} // namespace inet

#endif

