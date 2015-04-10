//
// Copyright (C) 2015 OpenSim Ltd.
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
// 

#ifndef __INET_IEEE80211EDCA_H
#define __INET_IEEE80211EDCA_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211eClassifier.h"

namespace inet {
namespace ieee80211 {

class Ieee80211Mac;

class INET_API Ieee80211Edca : public cSimpleModule
{
    public:
        typedef std::list<Ieee80211DataOrMgmtFrame *> Ieee80211DataOrMgmtFrameList;
        struct AccessCategory
        {
            simtime_t TXOP;
            bool backoff;
            simtime_t backoffPeriod;
            int retryCounter;
            int AIFSN;    // Arbitration interframe space number. The duration edcCAF[AC].AIFSis a duration derived from the value AIFSN[AC] by the relation
            int cwMax;
            int cwMin;
            // queue
            Ieee80211DataOrMgmtFrameList transmissionQueue;
            // per class timers
            cMessage *endAIFS;
            cMessage *endBackoff;
            /** @name Statistics per Access Class*/
            //@{
            long numRetry;
            long numSentWithoutRetry;
            long numGivenUp;
            long numSent;
            long numDropped;
            long bits;
            simtime_t minjitter;
            simtime_t maxjitter;
        };

        typedef std::vector<AccessCategory> EdcaVector;

        struct EdcaOutVector
        {
            cOutVector *jitter;
            cOutVector *macDelay;
            cOutVector *throughput;
        };

    protected:
        Ieee80211Mac *ieee80211Mac = nullptr;

        std::vector<AccessCategory> edcCAF;
        std::vector<EdcaOutVector> edcCAFOutVector;

        IQoSClassifier *classifier = nullptr;

        /** Default access catagory */
        int defaultAC = 0;

        /** Indicates which queue is acite. Depends on access category. */
        int currentAC = 0;

        /** Remember currentAC. We need this to figure out internal colision. */
        int oldcurrentAC = 0;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }
        virtual void initialize(int stage) override;
        void handleSelfMessage(cMessage *msg);
        virtual void finish() override;

        void initWatches();

    public:
        virtual ~Ieee80211Edca();

        // methods for access to specific AC data
        virtual bool& backoff(int i = -1);
        virtual simtime_t& TXOP(int i = -1);
        virtual simtime_t& backoffPeriod(int i = -1);
        virtual int& retryCounter(int i = -1);
        virtual int& AIFSN(int i = -1);
        virtual int& cwMax(int i = -1);
        virtual int& cwMin(int i = -1);
        virtual cMessage *endAIFS(int i = -1);
        virtual cMessage *endBackoff(int i = -1);
        virtual Ieee80211DataOrMgmtFrameList *transmissionQueue(int i = -1);
        virtual void setEndAIFS(int i, cMessage *msg) { edcCAF[i].endAIFS = msg; }
        virtual void setEndBackoff(int i, cMessage *msg) { edcCAF[i].endBackoff = msg; }

        // Statistics
        virtual long& numRetry(int i = -1);
        virtual long& numSentWithoutRetry(int i = -1);
        virtual long& numGivenUp(int i = -1);
        virtual long& numSent(int i = -1);
        virtual long& numDropped(int i = -1);
        virtual long& bits(int i = -1);
        virtual simtime_t& minJitter(int i = -1);
        virtual simtime_t& maxJitter(int i = -1);

        // out vectors
        virtual cOutVector *jitter(int i = -1);
        virtual cOutVector *macDelay(int i = -1);
        virtual cOutVector *throughput(int i = -1);
        inline int numCategories() const { return edcCAF.size(); }
        virtual const bool isBackoffMsg(cMessage *msg);

        int getCurrentAc() const { return currentAC; }
        void setCurrentAc(int currentAc) { currentAC = currentAc; }
        int getOldCurrentAc() const { return oldcurrentAC; }
        void setOldCurrentAc(int oldcurrentAc) { oldcurrentAC = oldcurrentAc; }

        void resetStateVariables();
        /** @brief Returns the current frame being transmitted */
        Ieee80211DataOrMgmtFrame *getCurrentTransmission();
        int mappingAccessCategory(Ieee80211DataOrMgmtFrame *frame);
        virtual bool transmissionQueueEmpty();
        virtual unsigned int transmissionQueueSize();
        const EdcaVector& getEdcCaf() const { return edcCAF; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __IEEE80211EDCA_H
