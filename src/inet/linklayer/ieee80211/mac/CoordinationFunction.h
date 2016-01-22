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

#ifndef __INET_COORDINATIONFUNCTION_H
#define __INET_COORDINATIONFUNCTION_H

#include "inet/linklayer/ieee80211/mac/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/fs/CompoundFrameSequences.h"
#include "inet/linklayer/ieee80211/mac/IContention.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"
#include "inet/linklayer/ieee80211/mac/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/IRx.h"
#include "inet/linklayer/ieee80211/mac/ITransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/ITx.h"
#include "inet/linklayer/ieee80211/mac/ITxCallback.h"
#include "inet/linklayer/ieee80211/mac/TimingParameters.h"
#include "inet/linklayer/ieee80211/mac/TxRetryHandler.h"

namespace inet {
namespace ieee80211 {

/**
 * Interface for IEEE 802.11 Coordination Functions.
 */
class INET_API ICoordinationFunction
{
};

/**
 * Base class for IEEE 802.11 Channel Access Functions.
 */
class INET_API CafBase : public cSimpleModule
{
    protected:
        IMacParameters *params = nullptr;
        MacUtils *utils = nullptr;
        TimingParameters *timingParameters = nullptr;
        ITx *tx = nullptr;
        IRx *rx = nullptr;
        IRateSelection *rateSelection = nullptr;
        IDuplicateDetector *duplicateDetector = nullptr;
        ITransmitLifetimeHandler *transmitLifetimeHandler = nullptr;
        TxRetryHandler *txRetryHandler = nullptr;
        AckHandler *ackHandler = nullptr;

        PendingQueue *pendingQueue = nullptr;
        InProgressFrames *inProgressFrames = nullptr;

        IContention *contention = nullptr;
        IContentionCallback *contentionCallback = nullptr;

        IFrameSequence *frameSequence = nullptr;
        FrameSequenceContext *context = nullptr;

        cMessage *receiveTimeout = nullptr;

    protected:
        virtual void initialize() override;
        virtual void handleMessage(cMessage *message) override;

    protected:
        virtual void qosDataFrameTransmissionFailed(Ieee80211DataFrame *failedFrame);
        virtual void nonQoSFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *failedFrame);
        virtual void rtsFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *protectedFrame);
        virtual void processTransmittedFrame(Ieee80211Frame *frame);
        virtual void processReceivedFrame(Ieee80211Frame *frame);

    protected:
        virtual void startContention();
        virtual void startContentionIfNecessary();

        virtual void startFrameSequenceStep();
        virtual void finishFrameSequenceStep();
        virtual void finishFrameSequence(bool ok);
        virtual void abortFrameSequence();

    public:
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame);
        virtual void lowerFrameReceived(Ieee80211Frame *frame);
        virtual void transmissionComplete();

        virtual bool isSequenceRunning() { return frameSequence != nullptr; }

        // TODO: kludge
        void setParamsAndUtils(IMacParameters *params, MacUtils *utils) { this->params = params; this->utils = utils; }
};

/**
 * Implements IEEE 802.11 Distributed Coordination Function.
 */
class INET_API Dcf : public CafBase, public ICoordinationFunction, public IContentionCallback
{
    protected:
        virtual void initialize() override;

    public:
        virtual void channelAccessGranted(int txIndex) override;
        virtual void internalCollision(int txIndex) override;
};

/**
 * Implements IEEE 802.11 Point Coordination Function.
 */
class INET_API Pcf : public ICoordinationFunction
{
    protected:
//        PcfFs *frameSequence = nullptr;
        FrameSequenceContext *context = nullptr;
};

/**
 * Implements IEEE 802.11 Mesh Coordination Function.
 */
class INET_API Mcf : public ICoordinationFunction
{
    protected:
        McfFs *frameSequence = nullptr;
        FrameSequenceContext *context = nullptr;
};

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access Function.
 */
class INET_API Edcaf : public CafBase, public IContentionCallback
{
    protected:
        AccessCategory ac = AccessCategory(-1);

    protected:
        virtual void initialize() override;

    public:
        virtual void channelAccessGranted(int txIndex) override;
        virtual void internalCollision(int txIndex) override;
};

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access.
 */
class INET_API Edca
{
    protected:
        std::vector<Edcaf *> edcafs;

    protected:
        virtual Edcaf *getActiveEdcaf();
        virtual AccessCategory classifyFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual AccessCategory mapTidToAc(int tid);

    public:
        Edca(IMacParameters *params, MacUtils *utils, ITx *tx, cModule *upperMac);

        virtual bool isSequenceRunning() { return getActiveEdcaf() != nullptr; }
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame);
        virtual void lowerFrameReceived(Ieee80211Frame *frame);
        virtual void transmissionComplete();
};

/**
 * Implements IEEE 802.11 Hybrid coordination function (HCF) Controlled Channel Access.
 */
class INET_API Hcca
{
};

/**
 * Implements IEEE 802.11 Hybrid Coordination Function.
 */
class INET_API Hcf : public ICoordinationFunction
{
    protected:
        Edca *edca;
        Hcca *hcca;

    public:
        Hcf(IMacParameters *params, MacUtils *utils, ITx *tx, cModule *upperMac);

        virtual bool isSequenceRunning() { return edca->isSequenceRunning(); }
        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame *frame);
        virtual void lowerFrameReceived(Ieee80211Frame *frame);
        virtual void transmissionComplete();
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_COORDINATIONFUNCTION_H
