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

#ifndef __INET__RECIPIENTMPDUHANDLER_H
#define __INET__RECIPIENTMPDUHANDLER_H

#include "inet/linklayer/ieee80211/mac/contract/IRecipientMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/ITx.h"
#include "inet/linklayer/ieee80211/mac/lifetime/DcfReceiveLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/recipient/AckProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/CtsProcedure.h"
#include "inet/linklayer/ieee80211/mac/recipient/RecipientMacDataService.h"

namespace inet {
namespace ieee80211 {

class Ieee80211Mac;

class INET_API RecipientMpduHandler : public IRecipientMpduHandler, public ITx::ICallback, public cSimpleModule
{
    protected:
        Ieee80211Mac *mac = nullptr;
        ITx *tx = nullptr;
        RecipientMacDataService *macDataService = nullptr;
        CtsProcedure *ctsProcedure = nullptr;
        AckProcedure *ackProcedure = nullptr;
        DcfReceiveLifetimeHandler *receiveLifetimeHandler = nullptr;

        simtime_t sifs = -1;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;

        void processControlFrame(Ieee80211Frame *frame);
        void sendUp(const std::vector<Ieee80211Frame*>& completeFrames);

    public:
        void processReceivedFrame(Ieee80211Frame *frame) override;
        void transmissionComplete() override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_RECIPIENTMPDUHANDLER_H
