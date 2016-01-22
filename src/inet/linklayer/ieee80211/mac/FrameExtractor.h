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

#ifndef __INET_FRAMEEXTRACTOR_H
#define __INET_FRAMEEXTRACTOR_H

#include "inet/linklayer/ieee80211/mac/DuplicateDetectors.h"
#include "inet/linklayer/ieee80211/mac/Fragmentation.h"
#include "inet/linklayer/ieee80211/mac/ITransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/MsduAggregation.h"

namespace inet {
namespace ieee80211 {

// TODO: rename/move to queue or mac base or what?
// move to InProgressFrames?
class INET_API FrameExtractor {
    protected:
        IDuplicateDetector *duplicateDetector = nullptr;
        IMsduAggregation *msduAggregator = nullptr;
        IFragmenter *fragmenter = nullptr;
        ITransmitLifetimeHandler *transmitLifetimeHandler = nullptr;
        int fragmentationThreshold = 2346; // TODO: remove

    public:
        FrameExtractor(IDuplicateDetector *duplicateDetector, IMsduAggregation *msduAggregator, IFragmenter *fragmenter, ITransmitLifetimeHandler *transmitLifetimeHandler);
        std::vector<Ieee80211DataOrMgmtFrame *> extractFramesToTransmit(cQueue *queue);
};

} // namespace ieee80211
} // namespace inet

#endif

