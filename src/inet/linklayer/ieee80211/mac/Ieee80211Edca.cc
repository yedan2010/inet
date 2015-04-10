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

#include "inet/common/INETUtils.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Edca.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"


namespace inet {
namespace ieee80211 {

Define_Module(Ieee80211Edca);

// method for access to the EDCA data

// methods for access to specific AC data
bool& Ieee80211Edca::backoff(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].backoff;
}

simtime_t& Ieee80211Edca::TXOP(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].TXOP;
}

simtime_t& Ieee80211Edca::backoffPeriod(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].backoffPeriod;
}

int& Ieee80211Edca::retryCounter(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].retryCounter;
}

int& Ieee80211Edca::AIFSN(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].AIFSN;
}

int& Ieee80211Edca::cwMax(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].cwMax;
}

int& Ieee80211Edca::cwMin(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].cwMin;
}

cMessage *Ieee80211Edca::endAIFS(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].endAIFS;
}

cMessage *Ieee80211Edca::endBackoff(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].endBackoff;
}

void Ieee80211Edca::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL)
    {
        const char *classifierClass = par("classifier");
        classifier = check_and_cast<IQoSClassifier *>(inet::utils::createOne(classifierClass));
        unsigned int numQueues = classifier->getNumQueues();
        for (int i = 0; i < numQueues; i++) {
            AccessCategory catEdca; // FIXME: ctor?
            catEdca.backoff = false;
            catEdca.backoffPeriod = -1;
            catEdca.retryCounter = 0;
            edcCAF.push_back(catEdca);
        }
        defaultAC = par("defaultAC");
        if (classifier && dynamic_cast<Ieee80211eClassifier *>(classifier))
            static_cast<Ieee80211eClassifier *>(classifier)->setDefaultClass(defaultAC);
        for (int i = 0; i < numCategories(); i++) {
            std::stringstream os;
            os << i;
            std::string strAifs = "AIFSN" + os.str();
            std::string strTxop = "TXOP" + os.str();
            if (hasPar(strAifs.c_str()) && hasPar(strTxop.c_str())) {
                AIFSN(i) = par(strAifs.c_str());
                TXOP(i) = par(strTxop.c_str());
            }
            else
                throw cRuntimeError("parameters %s , %s don't exist", strAifs.c_str(), strTxop.c_str());
        }
        if (numCategories() == 1)
            AIFSN(0) = par("AIFSN");
        unsigned int cwMinData = par("cwMinData");
        unsigned int cwMaxData = par("cwMaxData");
        for (int i = 0; i < numCategories(); i++) {
            ASSERT(AIFSN(i) >= 0 && AIFSN(i) < 16);
            if (i == 0 || i == 1) {
                cwMin(i) = cwMinData;
                cwMax(i) = cwMaxData;
            }
            if (i == 2) {
                cwMin(i) = (cwMinData + 1) / 2 - 1;
                cwMax(i) = cwMinData;
            }
            if (i == 3) {
                cwMin(i) = (cwMinData + 1) / 4 - 1;
                cwMax(i) = (cwMinData + 1) / 2 - 1;
            }
        }
        for (int i = 0; i < numCategories(); i++) {
            setEndAIFS(i, new cMessage("AIFS", i));
            setEndBackoff(i, new cMessage("Backoff", i));
        }
        currentAC = 0;
        oldcurrentAC = 0;
        for (int i = 0; i < numCategories(); i++)
            numDropped(i) = 0;
        // statistics
        for (int i = 0; i < numCategories(); i++) {
            numRetry(i) = 0;
            numSentWithoutRetry(i) = 0;
            numGivenUp(i) = 0;
            numSent(i) = 0;
            bits(i) = 0;
            maxJitter(i) = SIMTIME_ZERO;
            minJitter(i) = SIMTIME_ZERO;
        }
        for (int i = 0; i < numCategories(); i++) {
            EdcaOutVector outVectors;
            std::stringstream os;
            os << i;
            std::string th = "throughput AC" + os.str();
            std::string delay = "Mac delay AC" + os.str();
            std::string jit = "jitter AC" + os.str();
            outVectors.jitter = new cOutVector(jit.c_str());
            outVectors.throughput = new cOutVector(th.c_str());
            outVectors.macDelay = new cOutVector(delay.c_str());
            edcCAFOutVector.push_back(outVectors);
        }
        initWatches();
    }
}

void Ieee80211Edca::finish()
{
    for (int i = 0; i < numCategories(); i++) {
        std::stringstream os;
        os << i;
        std::string th = "number of retry for AC " + os.str();
        recordScalar(th.c_str(), numRetry(i));
    }
    for (int i = 0; i < numCategories(); i++) {
        std::stringstream os;
        os << i;
        std::string th = "sent packet within AC " + os.str();
        recordScalar(th.c_str(), numSent(i));
    }
    for (int i = 0; i < numCategories(); i++) {
        std::stringstream os;
        os << i;
        std::string th = "sentWithoutRetry AC " + os.str();
        recordScalar(th.c_str(), numSentWithoutRetry(i));
    }
    for (int i = 0; i < numCategories(); i++) {
        std::stringstream os;
        os << i;
        std::string th = "numGivenUp AC " + os.str();
        recordScalar(th.c_str(), numGivenUp(i));
    }
    for (int i = 0; i < numCategories(); i++) {
        std::stringstream os;
        os << i;
        std::string th = "numDropped AC " + os.str();
        recordScalar(th.c_str(), numDropped(i));
    }
}

Ieee80211Edca::~Ieee80211Edca()
{
    for (unsigned int i = 0; i < edcCAF.size(); i++) {
        cancelAndDelete(endAIFS(i));
        cancelAndDelete(endBackoff(i));
        while (!transmissionQueue(i)->empty()) {
            Ieee80211Frame *temp = dynamic_cast<Ieee80211Frame *>(transmissionQueue(i)->front());
            transmissionQueue(i)->pop_front();
            delete temp;
        }
    }
    edcCAF.clear();
    for (auto & elem : edcCAFOutVector) {
        delete elem.jitter;
        delete elem.macDelay;
        delete elem.throughput;
    }
    edcCAFOutVector.clear();
}

void Ieee80211Edca::initWatches()
{
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].retryCounter);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].backoff);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].backoffPeriod);
    WATCH(currentAC);
    WATCH(oldcurrentAC);
    for (int i = 0; i < numCategories(); i++)
        WATCH_LIST(edcCAF[i].transmissionQueue);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].numRetry);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].numSentWithoutRetry);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].numGivenUp);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].numSent);
    for (int i = 0; i < numCategories(); i++)
        WATCH(edcCAF[i].numDropped);
}

void Ieee80211Edca::handleSelfMessage(cMessage* msg)
{
    if (!strcmp(msg->getName(), "AIFS") || !strcmp(msg->getName(), "Backoff")) {
        EV_DEBUG << "Changing currentAC to " << msg->getKind() << endl;
        currentAC = msg->getKind();
    }
    //check internal collision
    if ((strcmp(msg->getName(), "Backoff") == 0) || (strcmp(msg->getName(), "AIFS") == 0)) {
        int kind;
        kind = msg->getKind();
        if (kind < 0)
            kind = 0;
        EV_DEBUG << " kind is " << kind << ",name is " << msg->getName() << endl;
        for (unsigned int i = numCategories() - 1; (int)i > kind; i--) {    //mozna prochaze jen 3..kind XXX
            if (((endBackoff(i)->isScheduled() && endBackoff(i)->getArrivalTime() == simTime())
                 || (endAIFS(i)->isScheduled() && !backoff(i) && endAIFS(i)->getArrivalTime() == simTime()))
                && !transmissionQueue(i)->empty())
            {
                EV_DEBUG << "Internal collision AC" << kind << " with AC" << i << endl;
                ieee80211Mac->increaseNumInternalCollision();
                EV_DEBUG << "Cancel backoff event and schedule new one for AC" << kind << endl;
                cancelEvent(endBackoff(kind));
                if (retryCounter() == ieee80211Mac->getTransmissionLimit() - 1) {
                    EV_WARN << "give up transmission for AC" << currentAC << endl;
                    ieee80211Mac->giveUpCurrentTransmission();
                }
                else {
                    EV_WARN << "retry transmission for AC" << currentAC << endl;
                    ieee80211Mac->retryCurrentTransmission();
                }
                return;
            }
        }
        currentAC = kind;
    }
}

int Ieee80211Edca::mappingAccessCategory(Ieee80211DataOrMgmtFrame *frame)
{
    bool isDataFrame = (dynamic_cast<Ieee80211DataFrame *>(frame) != nullptr);

    currentAC = classifier ? classifier->classifyPacket(frame) : 0;

    // check for queue overflow
    if (isDataFrame && ieee80211Mac->getMaxQueueSize() && (int)transmissionQueueSize() >= ieee80211Mac->getMaxQueueSize()) {
        EV_WARN << "message " << frame << " received from higher layer but AC queue is full, dropping message\n";
        numDropped()++;
        delete frame;
        return 200;
    }
    if (isDataFrame) {
        if (!ieee80211Mac->isPrioritizeMulticast() || !frame->getReceiverAddress().isMulticast() || transmissionQueue()->size() < 2)
            transmissionQueue()->push_back(frame);
        else {
            // current frame is multicast, insert it prior to any unicast dataframe
            ASSERT(transmissionQueue()->size() >= 2);
            auto p = transmissionQueue()->end();
            --p;
            while (p != transmissionQueue()->begin()) {
                if (dynamic_cast<Ieee80211DataFrame *>(*p) == nullptr)
                    break;  // management frame
                if ((*p)->getReceiverAddress().isMulticast())
                    break;  // multicast frame
                --p;
            }
            ++p;
            transmissionQueue()->insert(p, frame);
        }
    }
    else {
        if (transmissionQueue()->size() < 2) {
            transmissionQueue()->push_back(frame);
        }
        else {
            //we don't know if first frame in the queue is in middle of transmission
            //so for sure we placed it on second place
            auto p = transmissionQueue()->begin();
            p++;
            while ((p != transmissionQueue()->end()) && (dynamic_cast<Ieee80211DataFrame *>(*p) == nullptr)) // search the first not management frame
                p++;
            transmissionQueue()->insert(p, frame);
        }
    }
    EV_DEBUG << "frame classified as access category " << currentAC << " (0 background, 1 best effort, 2 video, 3 voice)\n";
    return true;
}

Ieee80211DataOrMgmtFrame *Ieee80211Edca::getCurrentTransmission()
{
    return transmissionQueue()->empty() ? nullptr : (Ieee80211DataOrMgmtFrame *)transmissionQueue()->front();
}

void Ieee80211Edca::resetStateVariables()
{
    backoffPeriod() = SIMTIME_ZERO;
    retryCounter() = 0;
    if (!transmissionQueue()->empty()) {
        backoff() = true;
        getCurrentTransmission()->setRetry(false);
    }
    else {
        backoff() = false;
    }
}


const bool Ieee80211Edca::isBackoffMsg(cMessage *msg)
{
    for (auto & elem : edcCAF) {
        if (msg == elem.endBackoff)
            return true;
    }
    return false;
}

// Statistics
long& Ieee80211Edca::numRetry(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].numRetry;
}

long& Ieee80211Edca::numSentWithoutRetry(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)numCategories())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].numSentWithoutRetry;
}

long& Ieee80211Edca::numGivenUp(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].numGivenUp;
}

long& Ieee80211Edca::numSent(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].numSent;
}

long& Ieee80211Edca::numDropped(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].numDropped;
}

long& Ieee80211Edca::bits(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].bits;
}

simtime_t& Ieee80211Edca::minJitter(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].minjitter;
}

simtime_t& Ieee80211Edca::maxJitter(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAF[i].maxjitter;
}

// out vectors

cOutVector *Ieee80211Edca::jitter(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAFOutVector[i].jitter;
}

cOutVector *Ieee80211Edca::macDelay(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAFOutVector[i].macDelay;
}

cOutVector *Ieee80211Edca::throughput(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return edcCAFOutVector[i].throughput;
}

Ieee80211Edca::Ieee80211DataOrMgmtFrameList *Ieee80211Edca::transmissionQueue(int i)
{
    if (i == -1)
        i = currentAC;
    if (i >= (int)edcCAF.size())
        throw cRuntimeError("AC doesn't exist");
    return &(edcCAF[i].transmissionQueue);
}

bool Ieee80211Edca::transmissionQueueEmpty()
{
    for (int i = 0; i < numCategories(); i++)
        if (!transmissionQueue(i)->empty())
            return false;

    return true;
}

unsigned int Ieee80211Edca::transmissionQueueSize()
{
    unsigned int totalSize = 0;
    for (int i = 0; i < numCategories(); i++)
        totalSize += transmissionQueue(i)->size();
    return totalSize;
}


} /* namespace ieee80211 */
} /* namespace inet */
