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

#include "inet/common/INETUtils.h"
#include "inet/linklayer/ieee80211/mac/aggregation/MsduAggregation.h"
#include "inet/linklayer/ieee80211/mac/coordinationfunction/CafBase.h"
#include "inet/linklayer/ieee80211/mac/duplicatedetector/QosDuplicateDetector.h"
#include "inet/linklayer/ieee80211/mac/fragmentation/Fragmentation.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"
#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceStep.h"

namespace inet {
namespace ieee80211 {

CafBase::~CafBase()
{
    cancelAndDelete(startReceptionTimeout);
    cancelAndDelete(endReceptionTimeout);
}

void CafBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        rx->registerContention(contention);
        endReceptionTimeout = new cMessage("endRxTimeout");
        startReceptionTimeout = new cMessage("startRxTimeout");
    }
}

void CafBase::handleMessage(cMessage* message)
{
    if (message == startReceptionTimeout) {
        auto lastStep = context->getLastStep();
        switch (lastStep->getType()) {
            case IFrameSequenceStep::Type::RECEIVE:
                if (!rx->isReceptionInProgress())
                    abortFrameSequence();
                break;
            case IFrameSequenceStep::Type::TRANSMIT:
                throw cRuntimeError("Received timeout while in transmit step");
            default:
                throw cRuntimeError("Unknown step type");
        }
    }
    else if (message == endReceptionTimeout) {
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

void CafBase::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    Enter_Method("upperFrameReceived(\"%s\")", frame->getName());
    take(frame);
    originatorMpduHandler->processUpperFrame(frame);
    // TODO: upperFrameProcessed()
    startContentionIfNecessary();
}

void CafBase::processLowerFrame(Ieee80211Frame* frame)
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
    // TODO: lowerFrameProcessed()
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
    int cw = originatorMpduHandler->getCw(); // FIXME: kludge
    contention->startContention(ifs, eifs, slotTime, cw, contentionCallback);
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
    // EV_INFO << "Frame sequence history:" << frameSequence->getHistory() << endl;
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
                tx->transmitFrame(transmitStep->getFrameToTransmit(), transmitStep->getIfs(), this);
                // TODO: lifetime
                // if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(transmitStep->getFrameToTransmit()))
                //    transmitLifetimeHandler->frameTransmitted(dataFrame);
                break;
            }
            case IFrameSequenceStep::Type::RECEIVE: {
                auto receiveStep = static_cast<ReceiveStep *>(nextStep);
                simtime_t earlyTimeout = receiveStep->getEarlyTimeout();
                if (earlyTimeout != -1)
                    scheduleAt(simTime() + earlyTimeout, startReceptionTimeout);
                // start reception timer, break loop if timer expires before reception is over
                simtime_t expectedDuration = receiveStep->getExpectedDuration();
                if (expectedDuration != -1) {
                    EV_INFO << "Receiving, timeout = " << simTime() + expectedDuration << "\n";
                    scheduleAt(simTime() + expectedDuration, endReceptionTimeout);
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
    // EV_INFO << "Frame sequence history:" << frameSequence->getHistory() << endl;
    if (!stepResult) {
        lastStep->setCompletion(IFrameSequenceStep::Completion::REJECTED);
        cancelEvent(endReceptionTimeout);
        abortFrameSequence();
    }
    else {
        lastStep->setCompletion(IFrameSequenceStep::Completion::ACCEPTED);
        switch (lastStep->getType()) {
            case IFrameSequenceStep::Type::TRANSMIT: {
                auto transmitStep = static_cast<TransmitStep *>(lastStep);
                originatorMpduHandler->processTransmittedFrame(transmitStep->getFrameToTransmit());
                break;
            }
            case IFrameSequenceStep::Type::RECEIVE: {
                auto receiveStep = static_cast<ReceiveStep *>(lastStep);
                auto transmitStep = check_and_cast<ITransmitStep*>(context->getStepBeforeLast());
                originatorMpduHandler->processReceivedFrame(receiveStep->getReceivedFrame(), transmitStep->getFrameToTransmit());
                cancelEvent(endReceptionTimeout);
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
    contention->releaseChannel();
    if (originatorMpduHandler->hasFrameToTransmit())
        startContentionIfNecessary();
}

void CafBase::abortFrameSequence()
{
    EV_INFO << "Frame sequence aborted\n";
    auto step = context->getLastStep();
    auto failedTxStep = check_and_cast<ITransmitStep*>(dynamic_cast<IReceiveStep*>(step) ? context->getStepBeforeLast() : step);
    auto frameToTransmit = failedTxStep->getFrameToTransmit();
    if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(frameToTransmit))
        originatorMpduHandler->processFailedFrame(dataOrMgmtFrame);
    else if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frameToTransmit)) {
        auto rtsTxStep = dynamic_cast<RtsTransmitStep*>(failedTxStep);
        originatorMpduHandler->processRtsProtectionFailed(rtsTxStep->getProtectedFrame());
        delete rtsFrame;
    }
    else if (auto blockAckReq = dynamic_cast<Ieee80211BlockAckReq *>(frameToTransmit))
        delete blockAckReq;
    else ; // TODO: etc ?

    for (int i = 0; i < context->getNumSteps(); i++) {
        if (auto txStep = dynamic_cast<ITransmitStep*>(context->getStep(i)))
            if (txStep != failedTxStep)
                delete txStep->getFrameToTransmit();
    }
    finishFrameSequence(false);
}

} // namespace ieee80211
} // namespace inet
