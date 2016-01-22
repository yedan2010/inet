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

#include "inet/linklayer/ieee80211/mac/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/FrameExtractor.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"

namespace inet {
namespace ieee80211 {

class INET_API InProgressFrames
{
    public:
        class SequenceControlPredicate
        {
            private:
                const std::set<std::pair<uint16_t, uint8_t>>& seqAndFragNums;
                std::vector<const Ieee80211DataOrMgmtFrame *> framesToDelete;

            public:
                SequenceControlPredicate(const std::set<std::pair<uint16_t, uint8_t>>& seqAndFragNums) :
                    seqAndFragNums(seqAndFragNums) {}

                bool operator() (const Ieee80211DataOrMgmtFrame *frame) {
                    bool ok = seqAndFragNums.count(std::make_pair(frame->getSequenceNumber(), frame->getFragmentNumber())) != 0;
                    if (ok) framesToDelete.push_back(frame);
                    return ok;
                }
                const std::vector<const Ieee80211DataOrMgmtFrame *>& getFramesToDelete() { return framesToDelete; }
        };

    protected:
        FrameExtractor *extractor = nullptr;
        PendingQueue *pendingQueue = nullptr;
        AckHandler *ackHandler = nullptr;
        ITransmitLifetimeHandler *transmitLifetimeHandler = nullptr;
        std::list<Ieee80211DataOrMgmtFrame *> inProgressFrames;

    protected:
        void ensureFilled();
        bool isEligibleStatusToTransmit(AckHandler::Status status);

    public:
        InProgressFrames(FrameExtractor *extractor, PendingQueue *pendingQueue, AckHandler *ackHandler, ITransmitLifetimeHandler *transmitLifetimeHandler) :
            extractor(extractor), pendingQueue(pendingQueue), ackHandler(ackHandler), transmitLifetimeHandler(transmitLifetimeHandler) { }

        virtual Ieee80211DataOrMgmtFrame *getFrameToTransmit();
        virtual void dropAndDeleteFrame(Ieee80211DataOrMgmtFrame *dataOrMgmtFrame);
        virtual void dropAndDeleteFrame(int seqNum, int fragNum);
        virtual void dropAndDeleteFrames(std::set<std::pair<uint16_t, uint8_t>> seqAndFragNums);

        virtual bool hasInProgressFrames() { return inProgressFrames.empty(); }
        virtual bool hasPendingFrames() { return pendingQueue->isEmpty(); }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_INPROGRESSFRAMES_H
