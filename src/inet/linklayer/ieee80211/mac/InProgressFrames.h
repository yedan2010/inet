//
// Copyright (C) 2015 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#ifndef __INET_INPROGRESSFRAMES_H
#define __INET_INPROGRESSFRAMES_H

#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMacDataService.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"

namespace inet {
namespace ieee80211 {

class INET_API InProgressFrames
{
    public:
        class SequenceControlPredicate
        {
            private:
                const std::set<SequenceControlField>& seqAndFragNums;
                std::vector<const Ieee80211DataOrMgmtFrame *> framesToDelete;

            public:
                SequenceControlPredicate(const std::set<SequenceControlField>& seqAndFragNums) :
                    seqAndFragNums(seqAndFragNums) {}

                bool operator() (const Ieee80211DataOrMgmtFrame *frame) {
                    bool ok = seqAndFragNums.count(SequenceControlField(frame->getSequenceNumber(), frame->getFragmentNumber())) != 0;
                    if (ok) framesToDelete.push_back(frame);
                    return ok;
                }
                const std::vector<const Ieee80211DataOrMgmtFrame *>& getFramesToDelete() { return framesToDelete; }
        };

    protected:
        PendingQueue *pendingQueue = nullptr;
        IOriginatorMacDataService *dataService = nullptr;
        AckHandler *ackHandler = nullptr;
        std::list<Ieee80211DataOrMgmtFrame *> inProgressFrames;

    protected:
        void ensureHasFrameToTransmit();
        bool isEligibleStatusToTransmit(AckHandler::Status status);
        bool hasEligibleFrameToTransmit();

    public:
        InProgressFrames(PendingQueue *pendingQueue, IOriginatorMacDataService *dataService, AckHandler *ackHandler) :
            pendingQueue(pendingQueue),
            dataService(dataService),
            ackHandler(ackHandler)
        { }

        virtual Ieee80211DataOrMgmtFrame *getFrameToTransmit();
        virtual void dropAndDeleteFrame(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame);
        virtual void dropAndDeleteFrame(int seqNum, int fragNum);
        virtual void dropAndDeleteFrames(std::set<SequenceControlField> seqAndFragNums);

        virtual bool hasInProgressFrames() { ensureHasFrameToTransmit(); return hasEligibleFrameToTransmit(); }
        virtual std::vector<Ieee80211DataFrame*> getOutstandingFrames();
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_INPROGRESSFRAMES_H
