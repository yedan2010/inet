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

#include "inet/linklayer/ieee80211/mac/FrameExtractor.h"

namespace inet {
namespace ieee80211 {

FrameExtractor::FrameExtractor(IDuplicateDetector *duplicateDetector, IMsduAggregation *msduAggregator, IFragmenter *fragmenter, ITransmitLifetimeHandler *transmitLifetimeHandler) :
    duplicateDetector(duplicateDetector),
    msduAggregator(msduAggregator),
    fragmenter(fragmenter),
    transmitLifetimeHandler(transmitLifetimeHandler)
{
}

std::vector<Ieee80211DataOrMgmtFrame *> FrameExtractor::extractFramesToTransmit(cQueue *queue)
{
    std::vector<Ieee80211DataOrMgmtFrame *> frames;
    if (msduAggregator == nullptr)
        frames.push_back(check_and_cast<Ieee80211DataOrMgmtFrame*>(queue->pop()));
    else
        frames.push_back(check_and_cast<Ieee80211DataOrMgmtFrame*>(msduAggregator->createAggregateFrame(queue)));
    duplicateDetector->assignSequenceNumber(frames.front());
    // TODO: The transmit MSDU timer shall be started when the MSDU is passed to the MAC.
    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(frames.front()))
        transmitLifetimeHandler->frameGotInProgess(dataFrame);
    auto nextDataFrame = dynamic_cast<Ieee80211DataFrame*>(frames.front());
    if (nextDataFrame && nextDataFrame->getAMsduPresent())
        EV_INFO << "The next frame is an " << frames.front()->getByteLength() << " octets long A-MSDU aggregated frame.\n";
    else if (frames.front()->getByteLength() > fragmentationThreshold) {
        EV_INFO << "The frame length is " << frames.front()->getByteLength()
                << " octets. Fragmentation threshold is reached. Fragmenting...\n";
        frames = fragmenter->fragment(frames.front(), fragmentationThreshold);
        EV_INFO << "The fragmentation process finished with " << frames.size() << "fragments.\n";
    }
    else
        EV_INFO << "The next frame is a simple frame.\n";
    return frames;
}

} // namespace ieee80211
} // namespace inet

