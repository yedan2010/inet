//
// Copyright (C) 2015 Andras Varga
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
// Author: Andras Varga
//

#include "inet/common/INETUtils.h"
#include "inet/linklayer/ieee80211/mac/CoordinationFunction.h"
#include "inet/linklayer/ieee80211/mac/DcfTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/EdcaTransmitLifetimeHandler.h"
#include "inet/linklayer/ieee80211/mac/fs/CompoundFrameSequences.h"

namespace inet {
namespace ieee80211 {

Define_Module(Dcf);
Define_Module(Edcaf);

// TODO: copy
inline double fallback(double a, double b) {return a!=-1 ? a : b;}
inline simtime_t fallback(simtime_t a, simtime_t b) {return a!=-1 ? a : b;}

void CafBase::initialize()
{
    tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
    rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
    rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
    duplicateDetector = check_and_cast<IDuplicateDetector *>(getModuleByPath(par("duplicateDetectorModule")));
    contention = check_and_cast<IContention *>(getSubmodule("contention"));
    pendingQueue = new PendingQueue(-1, nullptr);
    ackHandler = new AckHandler();
    inProgressFrames = new InProgressFrames(new FrameExtractor(duplicateDetector, nullptr, new BasicFragmentation(), transmitLifetimeHandler), pendingQueue, ackHandler, transmitLifetimeHandler);
    receiveTimeout = new cMessage("receiveTimer");
    rx->registerContention(contention);
}

void CafBase::nonQoSFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *failedFrame)
{
    ASSERT(failedFrame->getType() != ST_DATA_WITH_QOS);
    EV_INFO << "Data/Mgmt frame transmission failed\n";
    txRetryHandler->dataOrMgmtFrameTransmissionFailed(failedFrame);
    bool retryLimitReached = txRetryHandler->isDataOrMgtmFrameRetryLimitReached(failedFrame);
    if (retryLimitReached) {
        ackHandler->invalidateAckStatus(failedFrame);
        inProgressFrames->dropAndDeleteFrame(failedFrame);
    }
}

void CafBase::qosDataFrameTransmissionFailed(Ieee80211DataFrame* failedFrame)
{
    ASSERT(failedFrame->getType() == ST_DATA_WITH_QOS);
    ASSERT(failedFrame->getAckPolicy() != NO_ACK);
    EV_INFO << "QoS Data frame transmission failed\n";
    if (failedFrame->getAckPolicy() == NORMAL_ACK) {
        txRetryHandler->dataOrMgmtFrameTransmissionFailed(failedFrame);
        bool retryLimitReached = txRetryHandler->isDataOrMgtmFrameRetryLimitReached(failedFrame);
        if (retryLimitReached) {
            ackHandler->invalidateAckStatus(failedFrame);
            inProgressFrames->dropAndDeleteFrame(failedFrame);
        }
    }
    else if (failedFrame->getAckPolicy() == BLOCK_ACK) {
        bool lifetimeExpired = txRetryHandler->isLifetimeExpired(failedFrame);
        if (lifetimeExpired) {
            ackHandler->invalidateAckStatus(failedFrame);
            inProgressFrames->dropAndDeleteFrame(failedFrame);
        }
    }
    else
        throw cRuntimeError("Unimplemented!");
}

void CafBase::rtsFrameTransmissionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    EV_INFO << "RTS frame transmission failed\n";
    txRetryHandler->rtsFrameTransmissionFailed(protectedFrame);
    bool retryLimitReached = txRetryHandler->isRtsFrameRetryLimitReached(protectedFrame);
    if (retryLimitReached) {
        ackHandler->invalidateAckStatus(protectedFrame);
        inProgressFrames->dropAndDeleteFrame(protectedFrame);
    }
}

void CafBase::processTransmittedFrame(Ieee80211Frame* frame)
{
    if (utils->isBroadcastOrMulticast(frame))
        txRetryHandler->multicastFrameTransmitted();
    if (frame->getType() == ST_DATA || dynamic_cast<Ieee80211ManagementFrame *>(frame))
        ackHandler->processTransmittedNonQoSFrame(check_and_cast<Ieee80211DataOrMgmtFrame *>(frame));
    else if (frame->getType() == ST_DATA_WITH_QOS) {
        auto dataFrame = check_and_cast<Ieee80211DataFrame *>(frame);
        ackHandler->processTransmittedQoSData(dataFrame);
        if (dataFrame->getAckPolicy() == NO_ACK) {
            ackHandler->invalidateAckStatus(dataFrame);
            inProgressFrames->dropAndDeleteFrame(dataFrame);
        }
    }
    else if (auto blockAckReq = dynamic_cast<Ieee80211BlockAckReq*>(frame))
        ackHandler->processTransmittedBlockAckReq(blockAckReq);
    else
        // TODO: control frames, etc, void
        ;
}

void CafBase::processReceivedFrame(Ieee80211Frame* frame)
{
    if (frame->getType() == ST_ACK) {
        auto transmitStep = check_and_cast<ITransmitStep*>(context->getStepBeforeLast());
        auto ackedFrame = check_and_cast<Ieee80211DataOrMgmtFrame *>(transmitStep->getFrameToTransmit());
        txRetryHandler->ackFrameReceived(ackedFrame);
        ackHandler->processReceivedAck(check_and_cast<Ieee80211ACKFrame *>(frame), ackedFrame);
        ackHandler->invalidateAckStatus(ackedFrame);
        inProgressFrames->dropAndDeleteFrame(ackedFrame);
    }
    else if (frame->getType() == ST_BLOCKACK) {
        txRetryHandler->blockAckFrameReceived();
        auto ackedSeqAndFragNums = ackHandler->processReceivedBlockAck(check_and_cast<Ieee80211BlockAck *>(frame));
        ackHandler->invalidateAckStatuses(ackedSeqAndFragNums);
        inProgressFrames->dropAndDeleteFrames(ackedSeqAndFragNums);
    }
    else if (frame->getType() == ST_RTS)
        ; // void
    else if (frame->getType() == ST_CTS)
        txRetryHandler->ctsFrameReceived();
    else if (frame->getType() == ST_DATA_WITH_QOS)
        ; // void
    else if (frame->getType() == ST_BLOCKACK_REQ)
        ; // void
    else
        throw cRuntimeError("Unknown frame type");
}

void CafBase::handleMessage(cMessage* message)
{
    if (message == receiveTimeout) {
        auto lastStep = context->getLastStep();
        switch (lastStep->getType()) {
            case IFrameSequenceStep::Type::RECEIVE:
                lastStep->setCompletion(IFrameSequenceStep::Completion::EXPIRED);
                abortFrameSequence();
                break;
            case IFrameSequenceStep::Type::TRANSMIT:
                throw cRuntimeError("Received timeout while in transmit step");
            default:
                throw cRuntimeError("Unknown step type");
        }
    }
    else
        throw cRuntimeError("Unknown message");
}

void CafBase::upperFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    take(frame);
    pendingQueue->insert(frame);
    startContentionIfNecessary();
}

void CafBase::lowerFrameReceived(Ieee80211Frame* frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    auto lastStep = context->getLastStep();
    switch (lastStep->getType()) {
        case IFrameSequenceStep::Type::RECEIVE: {
            IReceiveStep *receiveStep = check_and_cast<IReceiveStep*>(context->getLastStep());
            receiveStep->setFrameToReceive(frame);
            finishFrameSequenceStep();
            if (isSequenceRunning())
                startFrameSequenceStep();
            break;
        }
        case IFrameSequenceStep::Type::TRANSMIT:
            throw cRuntimeError("Received frame while current step is transmit");
        default:
            throw cRuntimeError("Unknown step type");
    }
}

void CafBase::transmissionComplete()
{
    Enter_Method("transmissionComplete()");
    if (isSequenceRunning()) {
        finishFrameSequenceStep();
        if (isSequenceRunning())
            startFrameSequenceStep();
    }
}

void CafBase::startContention()
{
    EV_INFO << "Starting the contention\n";
    contention->startContention(timingParameters->getAifsTime(), timingParameters->getEifsTime(), timingParameters->getSlotTime(), txRetryHandler->getCw(), contentionCallback);
}

void CafBase::startContentionIfNecessary()
{
    if (!contention->isContentionInProgress())
        startContention();
    else
        EV_INFO << "Contention has already started\n";
}

void CafBase::startFrameSequenceStep()
{
    ASSERT(isSequenceRunning());
    auto nextStep = frameSequence->prepareStep(context);
//    EV_INFO << "Frame sequence history:" << frameSequence->getHistory() << endl;
    if (nextStep == nullptr)
        finishFrameSequence(true);
    else {
        context->addStep(nextStep);
        switch (nextStep->getType()) {
            case IFrameSequenceStep::Type::TRANSMIT: {
                auto transmitStep = static_cast<TransmitStep *>(nextStep);
                EV_INFO << "Transmitting, frame = " << transmitStep->getFrameToTransmit() << "\n";
                // The allowable frame exchange sequence is defined by the rule frame sequence. Except where modified by
                // the pifs attribute, frames are separated by a SIFS. (G.2 Basic sequences)
                tx->transmitFrame(inet::utils::dupPacketAndControlInfo(transmitStep->getFrameToTransmit()), transmitStep->getIfs());
                if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(transmitStep->getFrameToTransmit()))
                    transmitLifetimeHandler->frameTransmitted(dataFrame);
                break;
            }
            case IFrameSequenceStep::Type::RECEIVE: {
                auto receiveStep = static_cast<ReceiveStep *>(nextStep);
                // start reception timer, break loop if timer expires before reception is over
                simtime_t expectedDuration = receiveStep->getExpectedDuration();
                if (expectedDuration != -1) {
                    EV_INFO << "Receiving, timeout = " << simTime() + expectedDuration << "\n";
                    // TODO: ? simtime_t timeout = utils->getTimeout(expectedDuration, false); // TODO: pifs?, extend receive step if so!
                    simtime_t timeout = timingParameters->getSifsTime() + timingParameters->getSlotTime() + expectedDuration;
                    scheduleAt(simTime() + timeout, receiveTimeout);
                }
                break;
            }
            default:
                throw cRuntimeError("Unknown frame sequence step type");
        }
    }
}

void CafBase::finishFrameSequenceStep()
{
    ASSERT(isSequenceRunning());
    auto lastStep = context->getLastStep();
    auto stepResult = frameSequence->completeStep(context);
//    EV_INFO << "Frame sequence history:" << frameSequence->getHistory() << endl;
    if (!stepResult) {
        lastStep->setCompletion(IFrameSequenceStep::Completion::REJECTED);
        cancelEvent(receiveTimeout);
        abortFrameSequence();
    }
    else {
        lastStep->setCompletion(IFrameSequenceStep::Completion::ACCEPTED);
        switch (lastStep->getType()) {
            case IFrameSequenceStep::Type::TRANSMIT: {
                auto transmitStep = static_cast<TransmitStep *>(lastStep);
                processTransmittedFrame(transmitStep->getFrameToTransmit());
                break;
            }
            case IFrameSequenceStep::Type::RECEIVE: {
                auto receiveStep = static_cast<ReceiveStep *>(lastStep);
                processReceivedFrame(receiveStep->getReceivedFrame());
                cancelEvent(receiveTimeout);
                break;
            }
            default:
                throw cRuntimeError("Unknown frame sequence step type");
        }
    }
}

void CafBase::finishFrameSequence(bool ok)
{
    EV_INFO << (ok ? "Frame sequence finished\n" : "Frame sequence aborted\n");
    delete context;
    delete frameSequence;
    context = nullptr;
    frameSequence = nullptr;
    contention->channelReleased();
    if (!pendingQueue->isEmpty())
        startContentionIfNecessary();
}

void CafBase::abortFrameSequence()
{
    EV_INFO << "Frame sequence aborted\n";
    for (int i = 0; i < context->getNumSteps(); i++) {
        auto step = context->getStep(i);
        if (step->getType() == IFrameSequenceStep::Type::TRANSMIT) {
            auto transmitStep = check_and_cast<ITransmitStep *>(step);
            Ieee80211Frame *failedFrame = transmitStep->getFrameToTransmit();
            if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(failedFrame)) {
                if (ackHandler->getAckStatus(dataOrMgmtFrame) == AckHandler::Status::WAITING_FOR_RESPONSE) {
                    if (failedFrame->getKind() == ST_DATA || dynamic_cast<Ieee80211ManagementFrame*>(failedFrame))
                        nonQoSFrameTransmissionFailed(dataOrMgmtFrame);
                    else if (failedFrame->getKind() == ST_DATA_WITH_QOS)
                        qosDataFrameTransmissionFailed(check_and_cast<Ieee80211DataFrame*>(failedFrame));
                    else ; // void?
                }
            } // TODO: Ieee80211ControlFrame
            else if (failedFrame->getKind() == ST_RTS) {
                auto rtsTransmitStep = check_and_cast<RtsTransmitStep *>(step);
                rtsFrameTransmissionFailed(rtsTransmitStep->getProtectedFrame());
            }
            else if (failedFrame->getKind() == ST_BLOCKACK_REQ)
                /* TODO: BlockAckReq retransmission policy????????? */;
            else
                throw cRuntimeError("What to do? Probably ignore.");
        }
    }
    finishFrameSequence(false);
}

void Dcf::initialize()
{
    CafBase::initialize();
    contentionCallback = this;
    const IIeee80211Mode *referenceMode = rateSelection->getSlowestMandatoryMode(); // or any other; slotTime etc must be the same for all modes we use
    int aCwMin = referenceMode->getLegacyCwMin();
    int aCwMax = referenceMode->getLegacyCwMax();
    auto slotTime = fallback(par("slotTime"), referenceMode->getSlotTime());
    auto sifsTime = fallback(par("sifsTime"), referenceMode->getSifsTime());
    auto difsTime = fallback(par("difsTime"), referenceMode->getSifsTime() + 2 * params->getSlotTime());
    auto eifsTime = sifsTime + difsTime + referenceMode->getDuration(LENGTH_ACK);
    auto cwMin = fallback(par("cwMin"), aCwMin);
    auto cwMax = fallback(par("cwMax"), aCwMax);
    auto cwMulticast = fallback(par("cwMulticast"), aCwMin);
    timingParameters = new TimingParameters(slotTime, sifsTime, difsTime, eifsTime, -1, -1, cwMin, cwMax, cwMulticast, 0);
    txRetryHandler = new TxRetryHandler(params, timingParameters);
    transmitLifetimeHandler = new DcfTransmitLifetimeHandler(0); // TODO:  dot11MaxTransmitMSDULifetime
}

void Dcf::channelAccessGranted(int txIndex)
{
    if (!isSequenceRunning()) {
        context = new FrameSequenceContext(inProgressFrames, utils, params, timingParameters, nullptr);
        frameSequence = new DcfFs();
        frameSequence->startSequence(context, 0);
        startFrameSequenceStep();
    }
    else
        throw cRuntimeError("Channel access granted while a frame sequence is running");
}

void Dcf::internalCollision(int txIndex)
{
    throw cRuntimeError("Internal collision is impossible in DCF mode");
}

void Edcaf::initialize()
{
    CafBase::initialize();
    ac = AccessCategory(getIndex());
    contentionCallback = this;
    const IIeee80211Mode *referenceMode = rateSelection->getSlowestMandatoryMode(); // or any other; slotTime etc must be the same for all modes we use
    int aCwMin = referenceMode->getLegacyCwMin();
    int aCwMax = referenceMode->getLegacyCwMax();
    auto slotTime = fallback(par("slotTime"), referenceMode->getSlotTime());
    auto sifsTime = fallback(par("sifsTime"), referenceMode->getSifsTime());
    int aifsn = fallback(par("aifsn"), MacUtils::getAifsNumber(ac));
    auto aifsTime = sifsTime + aifsn * slotTime;
    auto eifsTime = sifsTime + aifsTime + referenceMode->getDuration(LENGTH_ACK);
    auto cwMin = fallback(par("cwMin"), MacUtils::getCwMin(ac, aCwMin));
    auto cwMax = fallback(par("cwMax"), MacUtils::getCwMax(ac, aCwMax, aCwMin));
    auto cwMulticast = fallback(par("cwMulticast"), MacUtils::getCwMin(ac, aCwMin));
    auto txOpLimit = fallback(par("txopLimit"), MacUtils::getTxopLimit(ac, referenceMode));
    timingParameters = new TimingParameters(slotTime, sifsTime, aifsTime, eifsTime, -1, -1, cwMin, cwMax, cwMulticast, txOpLimit);
    txRetryHandler = new TxRetryHandler(params, timingParameters);
    transmitLifetimeHandler = new EdcaTransmitLifetimeHandler(0,0,0,0); // TODO:  dot11EDCATableMSDULifetime
}

void Edcaf::channelAccessGranted(int txIndex)
{
    if (!isSequenceRunning()) {
        context = new FrameSequenceContext(inProgressFrames, utils, params, timingParameters, new TxOp(TxOp::Type::EDCA, timingParameters->getTxopLimit()));
        frameSequence = new HcfFs();
        frameSequence->startSequence(context, 0);
        startFrameSequenceStep();
    }
    else
        throw cRuntimeError("Channel access granted while a frame sequence is running");
}

void Edcaf::internalCollision(int txIndex)
{
    // Take the first frame from queue assuming it would have been sent
    Ieee80211DataOrMgmtFrame *internallyCollidedFrame = inProgressFrames->getFrameToTransmit();
    txRetryHandler->dataOrMgmtFrameTransmissionFailed(internallyCollidedFrame);
    if (!txRetryHandler->isDataOrMgtmFrameRetryLimitReached(internallyCollidedFrame))
        startContention();
    else {
        ackHandler->invalidateAckStatus(internallyCollidedFrame);
        inProgressFrames->dropAndDeleteFrame(internallyCollidedFrame);
    }
}

Edca::Edca(IMacParameters *params, MacUtils *utils, ITx *tx, cModule *upperMac)
{
    for (int ac = 0; ac < AC_NUMCATEGORIES; ac++) {
        auto edcaf = check_and_cast<Edcaf *>(upperMac->getSubmodule("edcaf", ac));
        edcaf->setParamsAndUtils(params, utils);
        edcafs.push_back(edcaf);
    }
}

Edcaf* Edca::getActiveEdcaf()
{
    for (auto edcaf : edcafs)
        if (edcaf->isSequenceRunning())
            return edcaf;
    return nullptr;
}

void Edca::upperFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    AccessCategory ac = classifyFrame(frame);
    edcafs[ac]->upperFrameReceived(frame);
}

void Edca::lowerFrameReceived(Ieee80211Frame* frame)
{
    getActiveEdcaf()->lowerFrameReceived(frame);
}

void Edca::transmissionComplete()
{
    getActiveEdcaf()->transmissionComplete();
}

AccessCategory Edca::classifyFrame(Ieee80211DataOrMgmtFrame *frame)
{
    ASSERT(frame->getType() == ST_DATA_WITH_QOS);
    Ieee80211DataFrame *dataFrame = check_and_cast<Ieee80211DataFrame*>(frame);
    return mapTidToAc(dataFrame->getTid());  // QoS frames: map TID to AC
}

AccessCategory Edca::mapTidToAc(int tid)
{
    // standard static mapping (see "UP-to-AC mappings" table in the 802.11 spec.)
    switch (tid) {
        case 1: case 2: return AC_BK;
        case 0: case 3: return AC_BE;
        case 4: case 5: return AC_VI;
        case 6: case 7: return AC_VO;
        default: throw cRuntimeError("No mapping from TID=%d to AccessCategory (must be in the range 0..7)", tid);
    }
}

Hcf::Hcf(IMacParameters *params, MacUtils *utils, ITx *tx, cModule *upperMac)
{
    edca = new Edca(params, utils, tx, upperMac);
}

void Hcf::upperFrameReceived(Ieee80211DataOrMgmtFrame* frame)
{
    edca->upperFrameReceived(frame);
}

void Hcf::lowerFrameReceived(Ieee80211Frame* frame)
{
    edca->lowerFrameReceived(frame);
}

void Hcf::transmissionComplete()
{
    edca->transmissionComplete();
}

} // namespace ieee80211
} // namespace inet

