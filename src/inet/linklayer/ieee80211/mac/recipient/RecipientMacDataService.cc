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

#include "RecipientMacDataService.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/LegacyDuplicateDetector.h"

namespace inet {
namespace ieee80211 {

Define_Module(RecipientMacDataService);

void RecipientMacDataService::initialize()
{
    duplicateRemoval = new LegacyDuplicateDetector();
    basicReassembly = new BasicReassembly();
}

Ieee80211DataOrMgmtFrame* RecipientMacDataService::defragment(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame)
{
    if (auto completeFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(basicReassembly->addFragment(dataOrMgmtFrame)))
        return completeFrame;
    else
        return nullptr;
}

std::vector<Ieee80211Frame*> RecipientMacDataService::dataOrMgmtFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    // TODO: A-MPDU Deaggregation, MPDU Header+CRC Validation, Address1 Filtering, Duplicate Removal, MPDU Decryption
    if (duplicateRemoval && duplicateRemoval->isDuplicate(frame))
        return std::vector<Ieee80211Frame*>();
    Ieee80211DataOrMgmtFrame *defragmentedFrame = nullptr;
    if (basicReassembly) { // FIXME: defragmentation
        defragmentedFrame = defragment(frame);
    }
    // TODO: MSDU Integrity, Replay Detection, A-MSDU Deagg., RX MSDU Rate Limiting
    return std::vector<Ieee80211Frame*>({defragmentedFrame});
}

std::vector<Ieee80211Frame*> RecipientMacDataService::dataFrameReceived(Ieee80211DataFrame* dataFrame)
{
    return dataOrMgmtFrameReceived(dataFrame);
}


std::vector<Ieee80211Frame*> RecipientMacDataService::managementFrameReceived(Ieee80211ManagementFrame* mgmtFrame)
{
    return dataOrMgmtFrameReceived(mgmtFrame);
}

std::vector<Ieee80211Frame*> RecipientMacDataService::controlFrameReceived(Ieee80211Frame* controlFrame)
{
    return std::vector<Ieee80211Frame*>(); // has nothing to do
}

} /* namespace ieee80211 */
} /* namespace inet */
