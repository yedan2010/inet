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

#include "inet/linklayer/ieee80211/mac/framesequence/DcfFs.h"
#include "inet/linklayer/ieee80211/mac/framesequence/PrimitiveFrameSequences.h"

namespace inet {
namespace ieee80211 {

DcfFs::DcfFs() :
    // Excerpt from G.2 Basic sequences (p. 2309)
    // frame-sequence =
    //   ( [ CTS ] ( Management + broadcast | Data + group ) ) |
    //   ( [ CTS | RTS CTS] {frag-frame ACK } last-frame ACK )
    AlternativesFs({new SequentialFs({new OptionalFs(new CtsFs(), OPTIONALFS_PREDICATE(isSelfCtsNeeded)),
                                      new AlternativesFs({/*...*/}, nullptr)}), // TODO: broadcast
                    new SequentialFs({new OptionalFs(new AlternativesFs({new CtsFs(), new RtsCtsFs(), /*...*/},
                                                                        ALTERNATIVESFS_SELECTOR(selectSelfCtsOrRtsCts)),
                                                     OPTIONALFS_PREDICATE(isCtsOrRtsCtsNeeded)),
                                      new RepeatingFs(new FragFrameAckFs(), REPEATINGFS_PREDICATE(hasMoreFragments)),
                                      new LastFrameAckFs()})},
                   ALTERNATIVESFS_SELECTOR(selectDcfSequence))
{
}

int DcfFs::selectDcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    bool managementSequence = isBroadcastManagementOrGroupDataSequenceNeeded(frameSequence, context);
    bool fragFrameSequence = isFragFrameSequenceNeeded(frameSequence, context);
    if (managementSequence) return 0;
    else if (fragFrameSequence) return 1;
    else throw cRuntimeError("One alternative must be chosen");
}

int DcfFs::selectSelfCtsOrRtsCts(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    bool selfCts = isSelfCtsNeeded(nullptr, context); // TODO
    bool rtsCts = isRtsCtsNeeded(nullptr, context); // TODO
    if (selfCts) return 0;
    else if (rtsCts) return 1;
    else throw cRuntimeError("One alternative must be chosen");
}

bool DcfFs::hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context)
{
    return context->getInProgressFrames()->hasInProgressFrames() && context->getInProgressFrames()->getFrameToTransmit()->getMoreFragments();
}

bool DcfFs::isSelfCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return false; // TODO: Implement
}

bool DcfFs::isRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return context->getInProgressFrames()->getFrameToTransmit()->getByteLength() > context->getRtsProcedure()->getRtsThreshold();
}

bool DcfFs::isCtsOrRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    bool selfCts = isSelfCtsNeeded(frameSequence, context);
    bool rtsCts = isRtsCtsNeeded(frameSequence, context);
    return selfCts || rtsCts;
}

bool DcfFs::isBroadcastManagementOrGroupDataSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    if (context->getInProgressFrames()->hasInProgressFrames()) {
        auto frame = context->getInProgressFrames()->getFrameToTransmit();
        return (dynamic_cast<Ieee80211ManagementFrame *>(frame) && frame->getReceiverAddress().isBroadcast()) ||
               (dynamic_cast<Ieee80211DataFrame *>(frame) && frame->getReceiverAddress().isMulticast());
    }
    else
        return false;
}

bool DcfFs::isFragFrameSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return context->getInProgressFrames()->hasInProgressFrames() && context->getInProgressFrames()->getFrameToTransmit()->getType() == ST_DATA;
}

} // namespace ieee80211
} // namespace inet

