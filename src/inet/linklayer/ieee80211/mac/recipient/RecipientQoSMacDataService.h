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

#ifndef __INET_RECIPIENTQOSMACDATASERVICE_H
#define __INET_RECIPIENTQOSMACDATASERVICE_H

#include "inet/linklayer/ieee80211/mac/blockackreordering/BlockAckReordering.h"
#include "inet/linklayer/ieee80211/mac/contract/IRecipientMacDataService.h"
#include "inet/linklayer/ieee80211/mac/contract/IDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/fragmentation/BasicReassembly.h"
#include "inet/linklayer/ieee80211/mac/contract/IDefragmentation.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientBlockAckAgreementHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/IMsduDeaggregation.h"

namespace inet {
namespace ieee80211 {

//
// Figure 5-1—MAC data plane architecture
//
class INET_API RecipientQoSMacDataService : public IRecipientMacDataService, public cSimpleModule
{
    protected:
        RecipientBlockAckAgreementHandler *blockAckAgreementHandler = nullptr;
        BasicReassembly *basicReassembly = nullptr; // FIXME: use Defragmentation

        // AMpduDeaggregation *aMpduDeaggregation = nullptr;
        // MpduHeaderAndCrcValidation *mpduHeaderAndCrcValidation = nullptr;
        // Address1Filtering *address1Filtering = nullptr;
        IDuplicateDetector *duplicateRemoval = nullptr; // TODO:
        // MpduDecryptionAndIntegrity *mpduDecryptionAndIntegrity = nullptr;
        BlockAckReordering *blockAckReordering = nullptr;
        IDefragmentation *defragmentation = nullptr;
        IMsduDeaggregation *aMsduDeaggregation = nullptr;
        // RxMsduRateLimiting *rxMsduRateLimiting = nullptr;

    protected:
        virtual void initialize() override;

        virtual Ieee80211DataFrame* defragment(std::vector<Ieee80211DataFrame *> completeFragments);
        Ieee80211ManagementFrame* defragment(Ieee80211ManagementFrame *mgmtFragment);

    public:
        virtual std::vector<Ieee80211Frame *> dataFrameReceived(Ieee80211DataFrame *dataFrame) override;
        virtual std::vector<Ieee80211Frame *> controlFrameReceived(Ieee80211Frame *controlFrame) override;
        virtual std::vector<Ieee80211Frame *> managementFrameReceived(Ieee80211ManagementFrame *mgmtFrame) override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_RECIPIENTQOSMACDATASERVICE_H
