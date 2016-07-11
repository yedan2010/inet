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

#include "inet/common/FSMA.h"
#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/ieee80211/mac/contention/Contention.h"
#include "inet/linklayer/ieee80211/mac/contract/IMacRadioInterface.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

simsignal_t Contention::stateChangedSignal = registerSignal("stateChanged");
// for @statistic; don't forget to keep synchronized the C++ enum and the runtime enum definition
Register_Enum(Contention::State,
        (Contention::IDLE,
         Contention::DEFER,
         Contention::IFS_AND_BACKOFF)
         );

Define_Module(Contention);

void Contention::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        backoffOptimization = par("backoffOptimization");
        lastIdleStartTime = simTime() - SimTime::getMaxTime() / 2;

        startTxEvent = new cMessage("startTx");

        fsm.setName("fsm");
        fsm.setState(IDLE, "IDLE");

        WATCH(ifs);
        WATCH(eifs);
        WATCH(slotTime);
        WATCH(endEifsTime);
        WATCH(backoffSlots);
        WATCH(scheduledTransmissionTime);
        WATCH(lastChannelBusyTime);
        WATCH(lastIdleStartTime);
        WATCH(backoffOptimizationDelta);
        WATCH(mediumFree);
        WATCH(backoffOptimization);
        updateDisplayString();
    }
    else if (stage == INITSTAGE_LAST) {
        if (!par("initialChannelBusy") && simTime() == 0)
            lastChannelBusyTime = simTime() - SimTime().getMaxTime() / 2;
    }
}

Contention::~Contention()
{
    cancelAndDelete(startTxEvent);
}

void Contention::startContention(int cw, simtime_t ifs, simtime_t eifs, simtime_t slotTime, ICallback *callback)
{
    ASSERT(ifs >= 0 && eifs >= 0 && slotTime >= 0 && cw >= 0);
    Enter_Method("startContention()");
    ASSERT(fsm.getState() == IDLE);
    this->ifs = ifs;
    this->eifs = eifs;
    this->slotTime = slotTime;
    this->callback = callback;
    backoffSlots = cw;
    handleWithFSM(START);
}

void Contention::handleWithFSM(EventType event)
{
    emit(stateChangedSignal, fsm.getState());
    EV_DEBUG << "handleWithFSM: processing event " << getEventName(event) << "\n";
    bool finallyReportChannelAccessGranted = false;
    FSMA_Switch(fsm) {
        FSMA_State(IDLE) {
            FSMA_Enter(mac->sendDownPendingRadioConfigMsg());
            FSMA_Event_Transition(Starting-IFS-and-Backoff,
                    event == START && mediumFree,
                    IFS_AND_BACKOFF,
                    scheduleTransmissionRequest();
                    );
            FSMA_Event_Transition(Busy,
                    event == START && !mediumFree,
                    DEFER,
                    ;
                    );
            FSMA_Ignore_Event(event==MEDIUM_STATE_CHANGED);
            FSMA_Ignore_Event(event==CORRUPTED_FRAME_RECEIVED);
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(DEFER) {
            FSMA_Enter(mac->sendDownPendingRadioConfigMsg());
            FSMA_Event_Transition(Restarting-IFS-and-Backoff,
                    event == MEDIUM_STATE_CHANGED && mediumFree,
                    IFS_AND_BACKOFF,
                    scheduleTransmissionRequest();
                    );
            FSMA_Event_Transition(Use-EIFS,
                    event == CORRUPTED_FRAME_RECEIVED,
                    DEFER,
                    endEifsTime = simTime() + eifs;
                    );
            FSMA_Fail_On_Unhandled_Event();
        }
        FSMA_State(IFS_AND_BACKOFF) {
            FSMA_Enter();
            FSMA_Event_Transition(Backoff-expired,
                    event == CHANNEL_ACCESS_GRANTED,
                    IDLE,
                    lastIdleStartTime = simTime();
                    finallyReportChannelAccessGranted = true;
                    );
            FSMA_Event_Transition(Defer-on-channel-busy,
                    event == MEDIUM_STATE_CHANGED && !mediumFree,
                    DEFER,
                    cancelTransmissionRequest();
                    computeRemainingBackoffSlots();
                    );
            FSMA_Event_Transition(Use-EIFS,
                    event == CORRUPTED_FRAME_RECEIVED,
                    IFS_AND_BACKOFF,
                    switchToEifs();
                    );
            FSMA_Fail_On_Unhandled_Event();
        }
    }
    emit(stateChangedSignal, fsm.getState());
    if (finallyReportChannelAccessGranted)
        callback->channelAccessGranted();
    if (hasGUI())
        updateDisplayString();
}

void Contention::mediumStateChanged(bool mediumFree)
{
    Enter_Method_Silent(mediumFree ? "medium FREE" : "medium BUSY");
    this->mediumFree = mediumFree;
    lastChannelBusyTime = simTime();
    handleWithFSM(MEDIUM_STATE_CHANGED);
}

void Contention::handleMessage(cMessage *msg)
{
    ASSERT(msg == startTxEvent);
    handleWithFSM(CHANNEL_ACCESS_GRANTED);
}

void Contention::corruptedFrameReceived()
{
    Enter_Method("corruptedFrameReceived()");
    handleWithFSM(CORRUPTED_FRAME_RECEIVED);
}

void Contention::scheduleTransmissionRequestFor(simtime_t txStartTime)
{
    scheduleAt(txStartTime, startTxEvent);
    callback->txStartTimeCalculated(txStartTime);
}

void Contention::cancelTransmissionRequest()
{
    cancelEvent(startTxEvent);
    callback->txStartTimeCanceled();
}

void Contention::scheduleTransmissionRequest()
{
    ASSERT(mediumFree);
    simtime_t now = simTime();
    bool useEifs = endEifsTime > now + ifs;
    simtime_t waitInterval = (useEifs ? eifs : ifs) + backoffSlots * slotTime;
    if (backoffOptimization && fsm.getState() == IDLE) {
        // we can pretend the frame has arrived into the queue a little bit earlier, and may be able to start transmitting immediately
        simtime_t elapsedFreeChannelTime = now - lastChannelBusyTime;
        simtime_t elapsedIdleTime = now - lastIdleStartTime;
        backoffOptimizationDelta = std::min(waitInterval, std::min(elapsedFreeChannelTime, elapsedIdleTime));
        if (backoffOptimizationDelta > SIMTIME_ZERO)
            waitInterval -= backoffOptimizationDelta;
    }
    scheduledTransmissionTime = now + waitInterval;
    scheduleTransmissionRequestFor(scheduledTransmissionTime);
}

void Contention::switchToEifs()
{
    endEifsTime = simTime() + eifs;
    cancelTransmissionRequest();
    scheduleTransmissionRequest();
}

void Contention::computeRemainingBackoffSlots()
{
    simtime_t remainingTime = scheduledTransmissionTime - simTime();
    int remainingSlots = (remainingTime.raw() + slotTime.raw() - 1) / slotTime.raw();
    if (remainingSlots < backoffSlots) // don't count IFS
        backoffSlots = remainingSlots;
}

void Contention::revokeBackoffOptimization()
{
    scheduledTransmissionTime += backoffOptimizationDelta;
    backoffOptimizationDelta = SIMTIME_ZERO;
    cancelTransmissionRequest();
    computeRemainingBackoffSlots();
    scheduleTransmissionRequest();
}

const char *Contention::getEventName(EventType event)
{
#define CASE(x)   case x: return #x;
    switch (event) {
        CASE(START);
        CASE(MEDIUM_STATE_CHANGED);
        CASE(CORRUPTED_FRAME_RECEIVED);
        CASE(CHANNEL_ACCESS_GRANTED);
        default: ASSERT(false); return "";
    }
#undef CASE
}

void Contention::updateDisplayString()
{
    const char *stateName = fsm.getStateName();
    if (strcmp(stateName, "IFS_AND_BACKOFF") == 0)
        stateName = "IFS+BKOFF";  // original text is too long
    getDisplayString().setTagArg("t", 0, stateName);
}

} // namespace ieee80211
} // namespace inet
