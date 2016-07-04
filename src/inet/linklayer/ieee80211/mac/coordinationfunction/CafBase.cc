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
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"

namespace inet {
namespace ieee80211 {

void CafBase::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        contention = check_and_cast<IContention *>(getSubmodule("contention"));
        rx->registerContention(contention);
        endReceptionTimeout = new cMessage("endRxTimeout");
        startReceptionTimeout = new cMessage("startRxTimeout");
    }
}

void CafBase::handleMessage(cMessage* message)
{
    if (message == endReceptionTimeout) {
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
                tx->transmitFrame(transmitStep->getFrameToTransmit(), transmitStep->getIfs(), this);
//                if (auto dataFrame = dynamic_cast<Ieee80211DataFrame *>(transmitStep->getFrameToTransmit()))
//                    transmitLifetimeHandler->frameTransmitted(dataFrame);
                break;
            }
            case IFrameSequenceStep::Type::RECEIVE: {
                auto receiveStep = static_cast<ReceiveStep *>(nextStep);
//                FIXME: implement early timeout
//                simtime_t earlyTimeout = receiveStep->getEarlyTimeout();
//                if (earlyTimeout != - 1)
//                    scheduleAt(simTime() + earlyTimeout, startReceptionTimeout);
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
//    EV_INFO << "Frame sequence history:" << frameSequence->getHistory() << endl;
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
    for (int i = 0; i < context->getNumSteps(); i++) {
        auto step = context->getStep(i);
        if (step->getType() == IFrameSequenceStep::Type::TRANSMIT) {
            auto transmitStep = check_and_cast<ITransmitStep *>(step);
            // FIXME: check if it is acked or not (txop)
            Ieee80211Frame *failedFrame = transmitStep->getFrameToTransmit();
            if (auto dataOrMgmtFrame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(failedFrame))
                originatorMpduHandler->processFailedFrame(dataOrMgmtFrame);
            // TODO: Ieee80211ControlFrame
            // TODO: originatorMpduHandler->processControlFrameFailed() ?
            else if (failedFrame->getType() == ST_RTS) {
                auto rtsTransmitStep = check_and_cast<RtsTransmitStep *>(step);
                originatorMpduHandler->processRtsProtectionFailed(rtsTransmitStep->getProtectedFrame());
            }
            else if (failedFrame->getType() == ST_BLOCKACK_REQ) // FIXME: remove
                /* TODO: BlockAckReq retransmission policy????????? */;
            else
                throw cRuntimeError("What to do? Probably ignore.");
        }
    }
    finishFrameSequence(false);
}

} // namespace ieee80211
} // namespace inet
