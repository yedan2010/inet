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

#include "inet/linklayer/ieee80211/mac/aggregation/MsduAggregation.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/QosDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/fragmentation/Fragmentation.h"
#include "OriginatorQoSMacDataService.h"

namespace inet {
namespace ieee80211 {

Define_Module(OriginatorQoSMacDataService);

void OriginatorQoSMacDataService::initialize()
{
    aMsduAggregationPolicy = check_and_cast<IMsduAggregationPolicy*>(getSubmodule("msduAggregationPolicy"));
    aMsduAggregation = new MsduAggregation();
    sequenceNumberAssigment = new QoSDuplicateDetector();
    fragmentationPolicy = check_and_cast<IFragmentationPolicy*>(getSubmodule("fragmentationPolicy"));
    fragmentation = new Fragmentation();
}

Ieee80211DataOrMgmtFrame *OriginatorQoSMacDataService::aMsduAggregateIfNeeded(PendingQueue *pendingQueue)
{
    auto subframes = aMsduAggregationPolicy->computeAggregateFrames(pendingQueue);
    if (subframes) {
        auto aggregatedFrame = aMsduAggregation->aggregateFrames(subframes);
        for (auto f : *subframes)
            pendingQueue->remove(f);
        delete subframes;
        return aggregatedFrame;
    }
    return nullptr;
}

Ieee80211DataOrMgmtFrame* OriginatorQoSMacDataService::assignSequenceNumber(Ieee80211DataOrMgmtFrame* frame)
{
    sequenceNumberAssigment->assignSequenceNumber(frame);
    return frame;
}

OriginatorQoSMacDataService::Fragments* OriginatorQoSMacDataService::fragmentIfNeeded(Ieee80211DataOrMgmtFrame* frame)
{
    auto fragmentSizes = fragmentationPolicy->computeFragmentSizes(frame);
    if (fragmentSizes) {
        auto fragmentFrames = fragmentation->fragmentFrame(frame, fragmentSizes);
        delete fragmentSizes;
        return fragmentFrames;
    }
    return nullptr;
}

OriginatorQoSMacDataService::Fragments* OriginatorQoSMacDataService::extractFramesToTransmit(PendingQueue *pendingQueue)
{
    if (pendingQueue->isEmpty())
        return nullptr;
    else {
        // if (msduRateLimiting)
        //    txRateLimitingIfNeeded();
        Ieee80211DataOrMgmtFrame *frame = nullptr;
        if (aMsduAggregationPolicy)
            frame = aMsduAggregateIfNeeded(pendingQueue);
        if (!frame)
            frame = pendingQueue->pop();
        // PS Defer Queueing
        if (sequenceNumberAssigment)
            frame = assignSequenceNumber(frame);
        // if (msduIntegrityAndProtection)
        //    frame = protectMsduIfNeeded(frame);
        Fragments *fragments = nullptr;
        if (fragmentationPolicy)
            fragments = fragmentIfNeeded(frame);
        if (!fragments)
            fragments = new Fragments({frame});
        // if (mpduEncryptionAndIntegrity)
        //    fragments = encryptMpduIfNeeded(fragments);
        // if (mpduHeaderPlusCrc)
        //    fragments = mpduCrcFooBarIfNeeded(fragments);
        // Ieee80211DataOrMgmtFrame *aMpdu = nullptr;
        // if (aMpduAggregation)
        //    aMpdu = aMpduAggregateIfNeeded(fragments);
        // if (aMpdu)
        //    fragments = new Fragments({aMpdu});
        return fragments;
    }
}

} /* namespace ieee80211 */
} /* namespace inet */
