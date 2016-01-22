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

#include "inet/linklayer/ieee80211/mac/fs/CompoundFrameSequences.h"
#include "inet/linklayer/ieee80211/mac/fs/PrimitiveFrameSequences.h"

namespace inet {
namespace ieee80211 {

#define OPTIONALFS_PREDICATE(predicate) [this](OptionalFs *frameSequence, FrameSequenceContext *context){ return predicate(frameSequence, context); }
#define REPEATINGFS_PREDICATE(predicate) [this](RepeatingFs *frameSequence, FrameSequenceContext *context){ return predicate(frameSequence, context); }
#define ALTERNATIVESFS_SELECTOR(selector) [this](AlternativesFs *frameSequence, FrameSequenceContext *context){ return selector(frameSequence, context); }

bool isSelfCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return false; // TODO: Implement
}

bool isRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return context->getFrameToTransmit()->getByteLength() > context->params->getRtsThreshold();
}

bool isPSPollNeeded(IFrameSequence *frameSequences, FrameSequenceContext *context)
{
    return false; // TODO: Implement!
}

bool isCtsOrRtsCtsOrPsPollNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    bool selfCts = isSelfCtsNeeded(frameSequence, context);
    bool rtsCts = isRtsCtsNeeded(frameSequence, context);
    bool PSPoll = isPSPollNeeded(frameSequence, context);
    if (selfCts || rtsCts || PSPoll)
        return true;
    else
        return false;
}

int selectSelfCtsOrRtsCtsOrPsPoll(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    bool selfCts = isSelfCtsNeeded(nullptr, context); // TODO
    bool rtsCts = isRtsCtsNeeded(nullptr, context); // TODO
    bool PSPoll = isPSPollNeeded(nullptr, context); // TODO
    if (selfCts) return 0;
    else if (rtsCts) return 1;
    else if (PSPoll) return 2;
    else throw cRuntimeError("One alternative must be chosen");
}

bool isBroadcastManagementOrGroupDataSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    if (context->hasFrameToTransmit()) {
        auto frame = context->getFrameToTransmit();
        return (dynamic_cast<Ieee80211ManagementFrame *>(frame) && frame->getReceiverAddress().isBroadcast()) ||
               (dynamic_cast<Ieee80211DataFrame *>(frame) && frame->getReceiverAddress().isMulticast());
    }
    else
        return false;
}

bool isFragFrameSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return context->hasFrameToTransmit() && context->getFrameToTransmit()->getType() == ST_DATA;
}

bool isPSPollAckSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return false; // TODO: Implement!
}

bool isBeaconSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return false; // TODO: Implement!
}

bool isHcfSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return context->hasFrameToTransmit() && context->getFrameToTransmit()->getType() == ST_DATA_WITH_QOS;
}

bool isMcfSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return false; // TODO: Implement!
}

TopLevelFs::TopLevelFs() :
//        G.2 Basic sequences (p. 2309)
//        frame-sequence =
//          ( [ CTS ] ( Management + broadcast | Data + group ) ) |
//          ( [ CTS | RTS CTS | PS-Poll ] {frag-frame ACK } last-frame ACK ) |
//          ( PS-Poll ACK ) |
//          ( [ Beacon + DTIM ] {cf-sequence} [ CF-End [+ CF-Ack ] ] )|
//          hcf-sequence |
//          mcf-sequence;
    // TODO: reuse DcfFs
    AlternativesFs({new SequentialFs({new OptionalFs(new CtsFs(), isSelfCtsNeeded),
                                      new AlternativesFs({/*...*/}, nullptr)}), // TODO: broadcast
                    new SequentialFs({new OptionalFs(new AlternativesFs({new CtsFs(), new RtsCtsFs(), /*...*/},
                                                                        selectSelfCtsOrRtsCtsOrPsPoll),
                                                     isCtsOrRtsCtsOrPsPollNeeded),
                                      new RepeatingFs(new FragFrameAckFs(), REPEATINGFS_PREDICATE(hasMoreFragments)),
                                      new LastFrameAckFs()}),
                    new SequentialFs({}), // TODO: poll
                    new SequentialFs({}), // TODO: beacon
                    new HcfFs(),
                    new McfFs()},
                   ALTERNATIVESFS_SELECTOR(selectTopLevelSequence))
{
}

int TopLevelFs::selectTopLevelSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    bool managementSequence = isBroadcastManagementOrGroupDataSequenceNeeded(frameSequence, context);
    bool fragFrameSequence = isFragFrameSequenceNeeded(frameSequence, context);
    bool PSPollAck = isPSPollAckSequenceNeeded(frameSequence, context);
    bool beaconSequence = isBeaconSequenceNeeded(frameSequence, context);
    bool hcfSequence = isHcfSequenceNeeded(frameSequence, context); // if there are a lot of frames in the queue for the recipient of the first frame
    bool mcfSequence = isMcfSequenceNeeded(frameSequence, context);
    if (managementSequence) return 0;
    else if (fragFrameSequence) return 1;
    else if (PSPollAck) return 2;
    else if (beaconSequence) return 3;
    else if (hcfSequence) return 4;
    else if (mcfSequence) return 5;
    else throw cRuntimeError("One alternative must be chosen");
}

bool TopLevelFs::hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context)
{
    return context->hasFrameToTransmit() && context->getFrameToTransmit()->getMoreFragments();
}

DcfFs::DcfFs() :
// Excerpt from G.2 Basic sequences (p. 2309)
// frame-sequence =
//   ( [ CTS ] ( Management + broadcast | Data + group ) ) |
//   ( [ CTS | RTS CTS] {frag-frame ACK } last-frame ACK )
    AlternativesFs({new SequentialFs({new OptionalFs(new CtsFs(), isSelfCtsNeeded),
                                      new AlternativesFs({/*...*/}, nullptr)}), // TODO: broadcast
                    new SequentialFs({new OptionalFs(new AlternativesFs({new CtsFs(), new RtsCtsFs(), /*...*/},
                                                                        selectSelfCtsOrRtsCtsOrPsPoll),
                                                     isCtsOrRtsCtsOrPsPollNeeded),
                                      new RepeatingFs(new FragFrameAckFs(), REPEATINGFS_PREDICATE(hasMoreFragments)),
                                      new LastFrameAckFs()}),
                    new SequentialFs({}), // TODO: poll
                    new SequentialFs({})}, // TODO: beacon
                   ALTERNATIVESFS_SELECTOR(selectDcfSequence))
{
}

int DcfFs::selectDcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    bool managementSequence = isBroadcastManagementOrGroupDataSequenceNeeded(frameSequence, context);
    bool fragFrameSequence = isFragFrameSequenceNeeded(frameSequence, context);
    bool PSPollAck = isPSPollAckSequenceNeeded(frameSequence, context);
    bool beaconSequence = isBeaconSequenceNeeded(frameSequence, context);
    if (managementSequence) return 0;
    else if (fragFrameSequence) return 1;
    else if (PSPollAck) return 2;
    else if (beaconSequence) return 3;
    else throw cRuntimeError("One alternative must be chosen");
}

bool DcfFs::hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context)
{
    return context->hasFrameToTransmit() && context->getFrameToTransmit()->getMoreFragments();
}

HcfFs::HcfFs() :
    // G.3 EDCA and HCCA sequences
    // hcf-sequence =
    //   ( [ CTS ] 1{( Data + group [+ QoS ] ) | Management + broadcast ) +pifs} |
    //   ( [ CTS ] 1{txop-sequence} ) |
    //   (* HC only, polled TXOP delivery *)
    //   ( [ RTS CTS ] non-cf-ack-piggybacked-qos-poll-sequence )
    //   (* HC only, polled TXOP delivery *)
    //   cf-ack-piggybacked-qos-poll-sequence |
    //   (* HC only, self TXOP delivery or termination *)
    //   Data + self + null + CF-Poll + QoS;
    AlternativesFs({new SequentialFs({new OptionalFs(new CtsFs(), OPTIONALFS_PREDICATE(isSelfCtsNeeded)),
                                      new RepeatingFs(new AlternativesFs({new DataFs(NORMAL_ACK), new ManagementFs()}, ALTERNATIVESFS_SELECTOR(selectDataOrManagementSequence)),
                                                      REPEATINGFS_PREDICATE(hasMoreTxOps))}),
                    new SequentialFs({new OptionalFs(new CtsFs(), OPTIONALFS_PREDICATE(isSelfCtsNeeded)),
                                      new RepeatingFs(new TxOpFs(), REPEATINGFS_PREDICATE(hasMoreTxOps))})},
                   ALTERNATIVESFS_SELECTOR(selectHcfSequence))
{
}

int HcfFs::selectHcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return 1;
}

int HcfFs::selectDataOrManagementSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return 0;
}

bool HcfFs::isSelfCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return false;
}

bool HcfFs::hasMoreTxOps(RepeatingFs *frameSequence, FrameSequenceContext *context)
{
    // TODO: if txOp time limit isn't yet reached and there are frames to the same recipient???? similarly to he isHcfSequenceNeeded
    return context->hasFrameToTransmit() && context->hasTxOp() && context->getTxOp()->getRemaining() > 0; // TODO: for edca context->timingParameters->getTxopLimit()
}

McfFs::McfFs() :
    // mcf-sequence =
    //   ( [ CTS ] |{( Data + group + QoS ) | Management + broadcast } ) | ( [ CTS ] 1{txop-sequence} ) |
    //   group-mccaop-abandon;
    AlternativesFs({/* TODO: */},
                   ALTERNATIVESFS_SELECTOR(selectMcfSequence))
{
}

int McfFs::selectMcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    return -1;
}

TxOpFs::TxOpFs() :
    // txop-sequence =
    //   ( ( ( RTS CTS ) | CTS + self ) Data + individual + QoS +( block-ack | no-ack ) ) |
    //   [ RTS CTS ] (txop-part-requiring-ack txop-part-providing-ack )|
    //   [ RTS CTS ] (Management | ( Data + QAP )) + individual ACK |
    //   [ RTS CTS ] (BlockAckReq BlockAck ) |
    //   ht-txop-sequence;
    AlternativesFs({new SequentialFs({new OptionalFs(new RtsCtsFs(), OPTIONALFS_PREDICATE(isDataRtsCtsNeeded)),
                                      new DataFs(BLOCK_ACK)}),
                    new SequentialFs({new OptionalFs(new RtsCtsFs(), OPTIONALFS_PREDICATE(isDataRtsCtsNeeded)),
                                      new DataFs(NORMAL_ACK),
                                      new AckFs()}),
                    new SequentialFs({new OptionalFs(new RtsCtsFs(), OPTIONALFS_PREDICATE(isBlockAckReqRtsCtsNeeded)),
                                      new BlockAckReqBlockAckFs()})},
                   ALTERNATIVESFS_SELECTOR(selectTxOpSequence))
{
}

int TxOpFs::selectTxOpSequence(AlternativesFs *frameSequence, FrameSequenceContext *context)
{
    // if there are too many? unacked frames for the recipient then return 1
    if (true) // context->originatorOutstandingFrames->size() >= 5
        return 2;
    else if (context->getFrameToTransmit()->getByteLength() > 1000)
        return 1;
    else
        return 0;
}

bool TxOpFs::isDataRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return context->getFrameToTransmit()->getByteLength() > context->params->getRtsThreshold();
}

bool TxOpFs::isBlockAckReqRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context)
{
    return false;
}

} // namespace ieee80211
} // namespace inet

