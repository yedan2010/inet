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

#include "inet/linklayer/ieee80211/mac/FrameSequenceContext.h"

namespace inet {
namespace ieee80211 {

FrameSequenceContext::FrameSequenceContext(InProgressFrames *inProgressFrames, MacUtils *utils, IMacParameters *params, TimingParameters *timingParameters, TxOp *txOp) :
    inProgressFrames(inProgressFrames),
    txOp(txOp),
    timingParameters(timingParameters),
    utils(utils),
    params(params)
{
}

FrameSequenceContext::~FrameSequenceContext()
{
    for (auto step : steps)
        delete step;
}

bool FrameSequenceContext::hasFrameToTransmit()
{
    return !inProgressFrames->hasInProgressFrames();
}

Ieee80211Frame *FrameSequenceContext::getFrameToTransmit()
{
    return inProgressFrames->getFrameToTransmit();
}

} // namespace ieee80211
} // namespace inet

