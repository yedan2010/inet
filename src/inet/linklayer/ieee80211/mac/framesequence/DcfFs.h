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
//

#ifndef __INET_DCFFS_H
#define __INET_DCFFS_H

#include "inet/linklayer/ieee80211/mac/framesequence/GenericFrameSequences.h"

namespace inet {
namespace ieee80211 {

class INET_API DcfFs : public AlternativesFs {
    public:
        DcfFs();

        virtual int selectDcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual int selectSelfCtsOrRtsCts(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context);
        virtual bool isSelfCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
        virtual bool isRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
        virtual bool isCtsOrRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
        virtual bool isBroadcastManagementOrGroupDataSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool isFragFrameSequenceNeeded(AlternativesFs *frameSequence, FrameSequenceContext *context);
};

} // namespace ieee80211
} // namespace inet

#endif
