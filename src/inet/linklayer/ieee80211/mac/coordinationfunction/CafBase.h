//
// Copyright (C) 2016 OpenSim Ltd.
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

#ifndef __INET_CAFBASE_H
#define __INET_CAFBASE_H

#include "inet/linklayer/ieee80211/mac/contract/IContention.h"
#include "inet/linklayer/ieee80211/mac/contract/IFrameSequence.h"
#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/contract/IRecipientMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/IRx.h"
#include "inet/linklayer/ieee80211/mac/contract/ITransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/ITx.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"

namespace inet {
namespace ieee80211 {

class Ieee80211Mac;

/**
 * Base class for IEEE 802.11 Channel Access Functions.
 * FIXME: implement early timeout
 */
class INET_API CafBase : public cSimpleModule, public ITx::ICallback
{
    protected:
        IOriginatorMpduHandler *originatorMpduHandler = nullptr;
        ITx *tx = nullptr;
        IRx *rx = nullptr;
        IContention *contention = nullptr;
        IContention::ICallback *contentionCallback = nullptr;
        IFrameSequence *frameSequence = nullptr;
        FrameSequenceContext *context = nullptr;
        cMessage *endReceptionTimeout = nullptr;
        cMessage *startReceptionTimeout = nullptr;

        simtime_t ifs = -1;
        simtime_t sifs = -1;
        simtime_t eifs = -1;
        simtime_t slotTime = -1;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *message) override;

        virtual void startContention();
        virtual void startContentionIfNecessary();

        virtual void startFrameSequenceStep();
        virtual void finishFrameSequenceStep();
        virtual void finishFrameSequence(bool ok);
        virtual void abortFrameSequence();

    public:
        ~CafBase();

        virtual void processUpperFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual void processLowerFrame(Ieee80211Frame *frame);
        virtual void transmissionComplete() override;

        virtual bool isSequenceRunning() { return frameSequence != nullptr; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_CAFBASE_H
