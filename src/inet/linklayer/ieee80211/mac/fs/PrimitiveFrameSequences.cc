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

#include "inet/linklayer/ieee80211/mac/fs/PrimitiveFrameSequences.h"

namespace inet {
namespace ieee80211 {

void CtsFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *CtsFs::prepareStep(FrameSequenceContext *context)
{
    // TODO:
    return nullptr;
}

bool CtsFs::completeStep(FrameSequenceContext *context)
{
    // TODO:
    return false;
}

DataFs::DataFs(int ackPolicy) :
    ackPolicy(ackPolicy)
{
}

void DataFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *DataFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            auto frame = check_and_cast<Ieee80211DataFrame *>(context->getFrameToTransmit());

            // TODO: hack for test
            frame->setType(ST_DATA_WITH_QOS);
            frame->setAckPolicy(ackPolicy);

            frame->setDuration(context->params->getSifsTime() + context->utils->getAckDuration());
            return new TransmitStep(frame, context->params->getSifsTime());
        }
        case 1:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool DataFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            step++;
            return true;
        default:
            throw cRuntimeError("Unknown step");
    }
}

void ManagementFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *ManagementFs::prepareStep(FrameSequenceContext *context)
{
    // TODO:
    return nullptr;
}

bool ManagementFs::completeStep(FrameSequenceContext *context)
{
    // TODO:
    return false;
}

void AckFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *AckFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            return new ReceiveStep();
        case 1:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool AckFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            auto receiveStep = check_and_cast<IReceiveStep*>(context->getStep(firstStep + step));
            step++;
            return receiveStep->getReceivedFrame()->getType() == ST_ACK;
        }
        default:
            throw cRuntimeError("Unknown step");
    }
}

void RtsCtsFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *RtsCtsFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            auto dataOrMgmtFrame = check_and_cast<Ieee80211DataOrMgmtFrame *>(context->getFrameToTransmit());
            auto rtsFrame = context->utils->buildRtsFrame(dataOrMgmtFrame);
            return new RtsTransmitStep(dataOrMgmtFrame, rtsFrame, context->params->getSifsTime());
        }
        case 1:
            return new ReceiveStep(context->utils->getCtsDuration());
        case 2:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool RtsCtsFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            step++;
            return true;
        case 1: {
            auto receiveStep = check_and_cast<IReceiveStep *>(context->getStep(firstStep + step));
            step++;
            return receiveStep->getReceivedFrame()->getType() == ST_CTS;
        }
        default:
            throw cRuntimeError("Unknown step");
    }
}

void FragFrameAckFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *FragFrameAckFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            auto frame = context->getFrameToTransmit();
            frame->setDuration(context->params->getSifsTime() + context->utils->getAckDuration());
            return new TransmitStep(frame, context->params->getSifsTime());
        }
        case 1:
            return new ReceiveStep(context->utils->getAckDuration());
        case 2:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool FragFrameAckFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            step++;
            return true;
        case 1: {
            auto receiveStep = check_and_cast<IReceiveStep *>(context->getStep(firstStep + step));
            step++;
            return receiveStep->getReceivedFrame()->getType() == ST_ACK;
        }
        default:
            throw cRuntimeError("Unknown step");
    }
}

void LastFrameAckFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *LastFrameAckFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            auto frame = context->getFrameToTransmit();
            frame->setDuration(context->params->getSifsTime() + context->utils->getAckDuration());
            return new TransmitStep(frame, context->params->getSifsTime());
        }
        case 1:
            return new ReceiveStep(context->utils->getAckDuration());
        case 2:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool LastFrameAckFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            step++;
            return true;
        case 1: {
            auto receiveStep = check_and_cast<IReceiveStep *>(context->getStep(firstStep + step));
            step++;
            return receiveStep->getReceivedFrame()->getType() == ST_ACK;
        }
        default:
            throw cRuntimeError("Unknown step");
    }
}

void BlockAckReqBlockAckFs::startSequence(FrameSequenceContext *context, int firstStep)
{
    this->firstStep = firstStep;
    step = 0;
}

IFrameSequenceStep *BlockAckReqBlockAckFs::prepareStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0: {
            // TODO: receiverAddress & startingSequenceNumber
            auto frame = context->getFrameToTransmit();
            auto receiverAddress = frame->getReceiverAddress();
            auto blockAckReqFrame = context->utils->buildBlockAckReqFrame(receiverAddress, 0);
            return new TransmitStep(blockAckReqFrame, context->params->getSifsTime());
        }
        case 1:
            return new ReceiveStep(context->utils->getCtsDuration());
        case 2:
            return nullptr;
        default:
            throw cRuntimeError("Unknown step");
    }
}

bool BlockAckReqBlockAckFs::completeStep(FrameSequenceContext *context)
{
    switch (step) {
        case 0:
            step++;
            return true;
        case 1: {
            auto receiveStep = check_and_cast<IReceiveStep*>(context->getStep(firstStep + step));
            step++;
            return receiveStep->getReceivedFrame()->getType() == ST_BLOCKACK;
        }
        default:
            throw cRuntimeError("Unknown step");
    }
}

} // namespace ieee80211
} // namespace inet

