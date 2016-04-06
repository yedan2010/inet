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

#ifndef __INET_ORIGINATORQOSMACDATASERVICE_H
#define __INET_ORIGINATORQOSMACDATASERVICE_H

#include "inet/linklayer/ieee80211/mac/contract/IDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/contract/IFragmentation.h"
#include "inet/linklayer/ieee80211/mac/contract/IFragmentationPolicy.h"
#include "inet/linklayer/ieee80211/mac/contract/IMsduAggregation.h"
#include "inet/linklayer/ieee80211/mac/contract/IMsduAggregationPolicy.h"
#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMacDataService.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"

namespace inet {
namespace ieee80211 {

class INET_API OriginatorQoSMacDataService : public IOriginatorMacDataService, public cSimpleModule
{
    protected:
        // Figure 5-1—MAC data plane architecture
        // MsduRateLimiting *msduRateLimiting = nullptr;
        // FIXME: separate sequence number assignment and duplicate detection
        IDuplicateDetector *sequenceNumberAssigment = nullptr;
        // MsduIntegrityAndProtection *msduIntegrityAndProtection = nullptr;
        // MpduEncryptionAndIntegrity *mpduEncryptionAndIntegrity = nullptr;
        // MpduHeaderPlusCrc *mpduHeaderPlusCrc = nullptr;
        IFragmentationPolicy *fragmentationPolicy = nullptr;
        IFragmentation *fragmentation = nullptr;
        IMsduAggregationPolicy *aMsduAggregationPolicy = nullptr;
        IMsduAggregation *aMsduAggregation = nullptr;
        // PsDeferQueueing *psDeferQueueing = nullptr;
        // AMpduAggregation *aMpduAggregation = nullptr;

    protected:
        virtual void initialize() override;

        virtual Ieee80211DataOrMgmtFrame *assignSequenceNumber(Ieee80211DataOrMgmtFrame *frame);
        virtual Fragments *fragmentIfNeeded(Ieee80211DataOrMgmtFrame *frame);
        virtual Ieee80211DataOrMgmtFrame *aMsduAggregateIfNeeded(PendingQueue *pendingQueue);

    public:
        virtual Fragments *extractFramesToTransmit(PendingQueue *pendingQueue) override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_ORIGINATORQOSMACDATASERVICE_H
