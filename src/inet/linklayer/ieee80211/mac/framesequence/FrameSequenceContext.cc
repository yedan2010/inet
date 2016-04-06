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

#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"

namespace inet {
namespace ieee80211 {

FrameSequenceContext::FrameSequenceContext(PendingQueue *pendingQueue, InProgressFrames *inProgressFrames, OriginatorAckProcedure *ackProcedure, RtsProcedure *rtsProcedure, TxOpProcedure *txopProcedure, OriginatorBlockAckProcedure *blockAckProcedure, OriginatorBlockAckAgreementHandler *blockAckAgreementHandler, const Ieee80211ModeSet *modeSet) :
    pendingQueue(pendingQueue),
    inProgressFrames(inProgressFrames),
    ackProcedure(ackProcedure),
    rtsProcedure(rtsProcedure),
    txopProcedure(txopProcedure),
    blockAckProcedure(blockAckProcedure),
    blockAckAgreementHandler(blockAckAgreementHandler),
    modeSet(modeSet)
{
}

FrameSequenceContext::~FrameSequenceContext()
{
    for (auto step : steps)
        delete step;
}

std::vector<Ieee80211DataFrame*> FrameSequenceContext::getOutstandingFrames()
{
    return inProgressFrames->getOutstandingFrames();
}

std::map<MACAddress, std::vector<Ieee80211DataFrame*>> FrameSequenceContext::getOutstandingFramesPerReceiver()
{
    auto outstandingFrames = getOutstandingFrames();
    std::map<MACAddress, std::vector<Ieee80211DataFrame*>> outstandingFramesPerReceiver;
    for (auto frame : outstandingFrames)
        outstandingFramesPerReceiver[frame->getReceiverAddress()].push_back(frame);
    return outstandingFramesPerReceiver;
}

} // namespace ieee80211
} // namespace inet

