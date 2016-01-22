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

#ifndef __INET_COMPOUNDFRAMESEQUENCES_H
#define __INET_COMPOUNDFRAMESEQUENCES_H

#include "inet/linklayer/ieee80211/mac/fs/GenericFrameSequences.h"

namespace inet {
namespace ieee80211 {

class INET_API TopLevelFs : public AlternativesFs {
    public:
        TopLevelFs();

        virtual int selectTopLevelSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context);
};

class INET_API DcfFs : public AlternativesFs {
    public:
        DcfFs();

        virtual int selectDcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool hasMoreFragments(RepeatingFs *frameSequence, FrameSequenceContext *context);
};

class INET_API HcfFs : public AlternativesFs {
    public:
        HcfFs();

        virtual int selectHcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual int selectDataOrManagementSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool isSelfCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
        virtual bool hasMoreTxOps(RepeatingFs *frameSequence, FrameSequenceContext *context);
};

class INET_API McfFs : public AlternativesFs {
    public:
        McfFs();

        virtual int selectMcfSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
};

class INET_API TxOpFs : public AlternativesFs {
    public:
        TxOpFs();

        virtual int selectTxOpSequence(AlternativesFs *frameSequence, FrameSequenceContext *context);
        virtual bool isDataRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
        virtual bool isBlockAckReqRtsCtsNeeded(OptionalFs *frameSequence, FrameSequenceContext *context);
};

} // namespace ieee80211
} // namespace inet

#endif

