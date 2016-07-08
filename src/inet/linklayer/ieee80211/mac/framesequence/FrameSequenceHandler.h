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

#ifndef __INET_FRAMESEQUENCEHANDLER_H
#define __INET_FRAMESEQUENCEHANDLER_H

#include "inet/linklayer/ieee80211/mac/contract/IFrameSequence.h"
#include "inet/linklayer/ieee80211/mac/contract/IOriginatorMpduHandler.h"
#include "inet/linklayer/ieee80211/mac/contract/ITransmitLifetimeHandler.h"

namespace inet {
namespace ieee80211 {

class INET_API FrameSequenceHandler : public IFrameSequenceHandler, public cSimpleModule
{
    protected:
        IOriginatorMpduHandler *originatorMpduHandler = nullptr;
        IFrameSequenceHandler::ICallback *callback = nullptr;
        IFrameSequence *frameSequence = nullptr;
        FrameSequenceContext *context = nullptr;
        cMessage *endReceptionTimeout = nullptr;
        cMessage *startReceptionTimeout = nullptr;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        virtual void handleMessage(cMessage *message) override;

        virtual void startFrameSequenceStep();
        virtual void finishFrameSequenceStep();
        virtual void finishFrameSequence(bool ok);
        virtual void abortFrameSequence();

    public:
        ~FrameSequenceHandler();

        virtual void startFrameSequence(IFrameSequence *frameSequence, FrameSequenceContext *context) override;
        virtual void processResponse(Ieee80211Frame *frame) override;
        virtual void transmissionComplete() override;
        virtual bool isSequenceRunning() { return frameSequence != nullptr; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_FRAMESEQUENCEHANDLER_H
