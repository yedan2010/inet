//
// Copyright (C) 2006 Andras Varga and Levente Meszaros
// Copyright (C) 2009 Lukáš Hlůže   lukas@hluze.cz (802.11e)
// Copyright (C) 2011 Alfonso Ariza  (clean code, fix some errors, new radio model)
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

#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "inet/physicallayer/contract/packetlevel/IRadio.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/physicallayer/contract/packetlevel/RadioControlInfo_m.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211ControlInfo_m.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211eClassifier.h"
#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"

namespace inet {

namespace ieee80211 {

// TODO: 9.3.2.1, If there are buffered multicast or broadcast frames, the PC shall transmit these prior to any unicast frames.
// TODO: control frames must send before

Define_Module(Ieee80211Mac);

// don't forget to keep synchronized the C++ enum and the runtime enum definition
Register_Enum(inet::Ieee80211Mac,
        (Ieee80211Mac::IDLE,
         Ieee80211Mac::DEFER,
         Ieee80211Mac::WAITAIFS,
         Ieee80211Mac::BACKOFF,
         Ieee80211Mac::WAITACK,
         Ieee80211Mac::WAITMULTICAST,
         Ieee80211Mac::WAITCTS,
         Ieee80211Mac::WAITSIFS,
         Ieee80211Mac::RECEIVE));

/****************************************************************
 * Construction functions.
 */

Ieee80211Mac::Ieee80211Mac()
{
}

Ieee80211Mac::~Ieee80211Mac()
{
    if (endSIFS) {
        delete (Ieee80211Frame *)endSIFS->getContextPointer();
        endSIFS->setContextPointer(nullptr);
        cancelAndDelete(endSIFS);
    }
    cancelAndDelete(endDIFS);
    cancelAndDelete(ackTimeout);
    cancelAndDelete(endReserve);
    cancelAndDelete(mediumStateChange);
    cancelAndDelete(endTXOP);
    if (pendingRadioConfigMsg)
        delete pendingRadioConfigMsg;
//    delete classifier;
}

/****************************************************************
 * Initialization functions.
 */

void Ieee80211Mac::initialize(int stage)
{
    EV_DEBUG << "Initializing stage " << stage << endl;

    MACProtocolBase::initialize(stage);

    //TODO: revise it: it's too big; should revise stages, too!!!
    if (stage == INITSTAGE_LOCAL) {
        autoRate = dynamic_cast<Ieee80211MacAutoRate *>(getSubmodule("autoRate"));
        edca = new Ieee80211Edca(); // TODO: module
        // initialize parameters
        modeSet = Ieee80211ModeSet::getModeSet(*par("opMode").stringValue());

        PHY_HEADER_LENGTH = par("phyHeaderLength");    //26us

        prioritizeMulticast = par("prioritizeMulticast");

        EV_DEBUG << "Operating mode: 802.11" << modeSet->getName();
        maxQueueSize = par("maxQueueSize");
        rtsThreshold = par("rtsThresholdBytes");

        // the variable is renamed due to a confusion in the standard
        // the name retry limit would be misleading, see the header file comment
        transmissionLimit = par("retryLimit");
        if (transmissionLimit == -1)
            transmissionLimit = 7;
        ASSERT(transmissionLimit >= 0);

        EV_DEBUG << " retryLimit=" << transmissionLimit;

        cwMinData = par("cwMinData");
        if (cwMinData == -1)
            cwMinData = CW_MIN;
        ASSERT(cwMinData >= 0 && cwMinData <= 32767);

        cwMaxData = par("cwMaxData");
        if (cwMaxData == -1)
            cwMaxData = CW_MAX;
        ASSERT(cwMaxData >= 0 && cwMaxData <= 32767);

        cwMinMulticast = par("cwMinMulticast");
        if (cwMinMulticast == -1)
            cwMinMulticast = 31;
        ASSERT(cwMinMulticast >= 0);
        EV_DEBUG << " cwMinMulticast=" << cwMinMulticast;

        ST = par("slotTime");    //added by sorin
        if (ST == -1)
            ST = 20e-6; //20us

        duplicateDetect = par("duplicateDetectionFilter");
        purgeOldTuples = par("purgeOldTuples");
        duplicateTimeOut = par("duplicateTimeOut");
        lastTimeDelete = 0;

        double bitrate = par("bitrate");
        if (bitrate == -1)
            dataFrameMode = modeSet->getFastestMode();
        else
            dataFrameMode = modeSet->getMode(bps(bitrate));

        double basicBitrate = par("basicBitrate");
        if (basicBitrate == -1)
            basicFrameMode = modeSet->getFastestMode();
        else
            basicFrameMode = modeSet->getMode(bps(basicBitrate));

        double controlBitRate = par("controlBitrate");
        if (controlBitRate == -1)
            controlFrameMode = modeSet->getSlowestMode();
        else
            controlFrameMode = modeSet->getMode(bps(controlBitRate));

        EV_DEBUG << " slotTime=" << getSlotTime() * 1e6 << "us DIFS=" << getDIFS() * 1e6 << "us";
        //end auto rate code
        EV_DEBUG << " basicBitrate=" << basicBitrate / 1e6 << "Mb ";
        EV_DEBUG << " bitrate=" << bitrate / 1e6 << "Mb IDLE=" << IDLE << " RECEIVE=" << RECEIVE << endl;

        const char *addressString = par("address");
        address = isInterfaceRegistered();
        if (address.isUnspecified()) {
            if (!strcmp(addressString, "auto")) {
                // assign automatic address
                address = MACAddress::generateAutoAddress();
                // change module parameter from "auto" to concrete address
                par("address").setStringValue(address.str().c_str());
            }
            else
                address.setAddress(addressString);
        }

        // initialize self messages
        endSIFS = new cMessage("SIFS");
        endDIFS = new cMessage("DIFS");
//        for (int i = 0; i < edca->numCategories(); i++) {
//            setEndAIFS(i, new cMessage("AIFS", i));
//            setEndBackoff(i, new cMessage("Backoff", i));
//        }
        endTXOP = new cMessage("TXOP");
        ackTimeout = new cMessage("Timeout");
        endReserve = new cMessage("Reserve");
        mediumStateChange = new cMessage("MediumStateChange");

        // obtain pointer to external queue
        initializeQueueModule();    //FIXME STAGE: this should be in L2 initialization!!!!

        // state variables
        fsm.setName("Ieee80211Mac State Machine");
        mode = DCF;
        sequenceNumber = 0;

        lastReceiveFailed = false;
        nav = false;
        txop = false;
        last = 0;

        numCollision = 0;
        numInternalCollision = 0;
        numReceived = 0;
        numSentMulticast = 0;
        numReceivedMulticast = 0;
        numBits = 0;
        numSentTXOP = 0;
        numReceivedOther = 0;
        numAckSend = 0;

        stateVector.setName("State");
        stateVector.setEnum("inet::Ieee80211Mac");
        // Code to compute the throughput over a period of time
        throughputTimePeriod = par("throughputTimePeriod");
        recBytesOverPeriod = 0;
        throughputLastPeriod = 0;
        throughputTimer = nullptr;
        if (throughputTimePeriod > 0)
            throughputTimer = new cMessage("throughput-timer");
        if (throughputTimer)
            scheduleAt(simTime() + throughputTimePeriod, throughputTimer);
        // end initialize variables throughput over a period of time
        // initialize watches
        validRecMode = false;
        initWatches();

        cModule *radioModule = gate("lowerLayerOut")->getNextGate()->getOwnerModule();
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (isOperational)
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
        // interface
        if (isInterfaceRegistered().isUnspecified()) //TODO do we need multi-MAC feature? if so, should they share interfaceEntry??  --Andras
            registerInterface();
    }
}

void Ieee80211Mac::initWatches()
{
// initialize watches
    WATCH(fsm);
    WATCH(nav);
    WATCH(txop);
    WATCH(numCollision);
    WATCH(numBits);
    WATCH(numSentTXOP);
    WATCH(numReceived);
    WATCH(numSentMulticast);
    WATCH(numReceivedMulticast);
    if (throughputTimer)
        WATCH(throughputLastPeriod);
}

void Ieee80211Mac::finish()
{
    recordScalar("number of received packets", numReceived);
    recordScalar("number of collisions", numCollision);
    recordScalar("number of internal collisions", numInternalCollision);
    recordScalar("sent and received bits", numBits);
    recordScalar("sent in TXOP ", numSentTXOP);
}

InterfaceEntry *Ieee80211Mac::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);

    // address
    e->setMACAddress(address);
    e->setInterfaceToken(address.formInterfaceIdentifier());

    e->setMtu(par("mtu").longValue());

    // capabilities
    e->setBroadcast(true);
    e->setMulticast(true);
    e->setPointToPoint(false);

    return e;
}

void Ieee80211Mac::initializeQueueModule()
{
    // use of external queue module is optional -- find it if there's one specified
    if (par("queueModule").stringValue()[0]) {
        cModule *module = getModuleFromPar<cModule>(par("queueModule"), this);
        queueModule = check_and_cast<IPassiveQueue *>(module);

        EV_DEBUG << "Requesting first two frames from queue module\n";
        queueModule->requestPacket();
        // needed for backoff: mandatory if next message is already present
        queueModule->requestPacket();
    }
}

/****************************************************************
 * Message handling functions.
 */
void Ieee80211Mac::handleSelfMessage(cMessage *msg)
{
    if (msg == throughputTimer) {
        throughputLastPeriod = recBytesOverPeriod / SIMTIME_DBL(throughputTimePeriod);
        recBytesOverPeriod = 0;
        scheduleAt(simTime() + throughputTimePeriod, throughputTimer);
        return;
    }

    EV_DEBUG << "received self message: " << msg << "(kind: " << msg->getKind() << ")" << endl;

    if (msg == endReserve)
        nav = false;

    if (msg == endTXOP)
        txop = false;
    handleWithFSM(msg);
}

void Ieee80211Mac::handleUpperPacket(cPacket *msg)
{
    if (queueModule && edca->numCategories() > 1 && (int)edca->transmissionQueueSize() < maxQueueSize) {
        // the module are continuously asking for packets, except if the queue is full
        EV_DEBUG << "requesting another frame from queue module\n";
        queueModule->requestPacket();
    }

    // check if it's a command from the mgmt layer
    if (msg->getBitLength() == 0 && msg->getKind() != 0) {
        handleUpperCommand(msg);
        return;
    }

    // must be a Ieee80211DataOrMgmtFrame, within the max size because we don't support fragmentation
    Ieee80211DataOrMgmtFrame *frame = check_and_cast<Ieee80211DataOrMgmtFrame *>(msg);
    if (frame->getByteLength() > fragmentationThreshold)
        throw cRuntimeError("message from higher layer (%s)%s is too long for 802.11b, %d bytes (fragmentation is not supported yet)",
                msg->getClassName(), msg->getName(), (int)(msg->getByteLength()));
    EV_DEBUG << "frame " << frame << " received from higher layer, receiver = " << frame->getReceiverAddress() << endl;

    // if you get error from this assert check if is client associated to AP
    ASSERT(!frame->getReceiverAddress().isUnspecified());

    // fill in missing fields (receiver address, seq number), and insert into the queue
    frame->setTransmitterAddress(address);
    frame->setSequenceNumber(sequenceNumber);
    sequenceNumber = (sequenceNumber + 1) % 4096;    //XXX seqNum must be checked upon reception of frames!

    if (edca->mappingAccessCategory(frame) == 200) {
        // if function mappingAccessCategory() returns 200, it means transsmissionQueue is full
        return;
    }
    frame->setMACArrive(simTime());
    handleWithFSM(frame);
}

void Ieee80211Mac::handleUpperCommand(cMessage *msg)
{
    if (msg->getKind() == RADIO_C_CONFIGURE) {
        EV_DEBUG << "Passing on command " << msg->getName() << " to physical layer\n";
        if (pendingRadioConfigMsg != nullptr) {
            // merge contents of the old command into the new one, then delete it
            ConfigureRadioCommand *oldConfigureCommand = check_and_cast<ConfigureRadioCommand *>(pendingRadioConfigMsg->getControlInfo());
            ConfigureRadioCommand *newConfigureCommand = check_and_cast<ConfigureRadioCommand *>(msg->getControlInfo());
            if (newConfigureCommand->getChannelNumber() == -1 && oldConfigureCommand->getChannelNumber() != -1)
                newConfigureCommand->setChannelNumber(oldConfigureCommand->getChannelNumber());
            if (isNaN(newConfigureCommand->getBitrate().get()) && !isNaN(oldConfigureCommand->getBitrate().get()))
                newConfigureCommand->setBitrate(oldConfigureCommand->getBitrate());
            delete pendingRadioConfigMsg;
            pendingRadioConfigMsg = nullptr;
        }

        if (fsm.getState() == IDLE || fsm.getState() == DEFER || fsm.getState() == BACKOFF) {
            EV_DEBUG << "Sending it down immediately\n";
/*
   // Dynamic power
            PhyControlInfo *phyControlInfo = dynamic_cast<PhyControlInfo *>(msg->getControlInfo());
            if (phyControlInfo)
                phyControlInfo->setAdaptiveSensitivity(true);
   // end dynamic power
 */
            sendDown(msg);
        }
        else {
            EV_DEBUG << "Delaying " << msg->getName() << " until next IDLE or DEFER state\n";
            pendingRadioConfigMsg = msg;
        }
    }
    else {
        throw cRuntimeError("Unrecognized command from mgmt layer: (%s)%s msgkind=%d", msg->getClassName(), msg->getName(), msg->getKind());
    }
}

void Ieee80211Mac::handleLowerPacket(cPacket *msg)
{
    EV_TRACE << "->Enter handleLowerMsg...\n";
    EV_DEBUG << "received message from lower layer: " << msg << endl;
    Ieee80211ReceptionIndication *cinfo = dynamic_cast<Ieee80211ReceptionIndication *>(msg->getControlInfo());
    if (cinfo && cinfo->getAirtimeMetric()) {
        double rtsTime = 0;
        if (rtsThreshold * 8 < cinfo->getTestFrameSize())
             rtsTime = controlFrameTxTime(LENGTH_CTS).dbl() + controlFrameTxTime(LENGTH_RTS).dbl(); // FIXME: simtime->dbl
        double frameDuration = cinfo->getTestFrameDuration() + controlFrameTxTime(LENGTH_ACK).dbl() + rtsTime; // FIXME: simtime->dbl
        cinfo->setTestFrameDuration(frameDuration);
    }
    emit(NF_LINK_FULL_PROMISCUOUS, msg);
    validRecMode = false;
    if (cinfo) {
        recFrameModulation = cinfo->getMode();
        if (!isNaN(recFrameModulation->getDataMode()->getNetBitrate().get()))
            validRecMode = true;
    }

    if (autoRate == nullptr) {
        if (msg->getControlInfo())
            delete msg->removeControlInfo();
    }

    Ieee80211Frame *frame = dynamic_cast<Ieee80211Frame *>(msg);
    if (autoRate)
        autoRate->someKindOfFunction1(frame);
    if (frame && throughputTimer)
        recBytesOverPeriod += frame->getByteLength();

    if (!frame) {
        EV_ERROR << "message from physical layer (%s)%s is not a subclass of Ieee80211Frame" << msg->getClassName() << " " << msg->getName() << endl;
        delete msg;
        return;
        // throw cRuntimeError("message from physical layer (%s)%s is not a subclass of Ieee80211Frame",msg->getClassName(), msg->getName());
    }

    EV_DEBUG << "Self address: " << address
             << ", receiver address: " << frame->getReceiverAddress()
             << ", received frame is for us: " << isForUs(frame)
             << ", received frame was sent by us: " << isSentByUs(frame) << endl;

    Ieee80211TwoAddressFrame *twoAddressFrame = dynamic_cast<Ieee80211TwoAddressFrame *>(msg);
    ASSERT(!twoAddressFrame || twoAddressFrame->getTransmitterAddress() != address);

    handleWithFSM(msg);

    // if we are the owner then we did not send this message up
    if (msg->getOwner() == this)
        delete msg;
    EV_TRACE << "Leave handleLowerMsg...\n";
}

void Ieee80211Mac::receiveSignal(cComponent *source, simsignal_t signalID, long value)
{
    Enter_Method_Silent();
    if (signalID == IRadio::receptionStateChangedSignal)
        handleWithFSM(mediumStateChange);
    else if (signalID == IRadio::transmissionStateChangedSignal) {
        handleWithFSM(mediumStateChange);
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
            configureRadioMode(IRadio::RADIO_MODE_RECEIVER);
        transmissionState = newRadioTransmissionState;
    }
}

/**
 * Msg can be upper, lower, self or nullptr (when radio state changes)
 */
void Ieee80211Mac::handleWithFSM(cMessage *msg)
{
    ASSERT(!isInHandleWithFSM);
    isInHandleWithFSM = true;
    removeOldTuplesFromDuplicateMap();
    // skip those cases where there's nothing to do, so the switch looks simpler
    if (isUpperMessage(msg) && fsm.getState() != IDLE) {
        if (fsm.getState() == WAITAIFS && endDIFS->isScheduled()) {
            // a difs was schedule because all queues ware empty
            // change difs for aifs
            simtime_t remaint = getAIFS(edca->getCurrentAc()) - getDIFS();
            scheduleAt(endDIFS->getArrivalTime() + remaint, edca->endAIFS(edca->getCurrentAc()));
            cancelEvent(endDIFS);
        }
        else if (fsm.getState() == BACKOFF && edca->endBackoff(edca->numCategories() - 1)->isScheduled() && edca->transmissionQueue(edca->numCategories() - 1)->empty()) {
            // a backoff was schedule with all the queues empty
            // reschedule the backoff with the appropriate AC
            edca->backoffPeriod(edca->getCurrentAc()) = edca->backoffPeriod(edca->numCategories() - 1);
            edca->backoff(edca->getCurrentAc()) = edca->backoff(edca->numCategories() - 1);
            edca->backoff(edca->numCategories() - 1) = false;
            scheduleAt(edca->endBackoff(edca->numCategories() - 1)->getArrivalTime(), edca->endBackoff(edca->getCurrentAc()));
            cancelEvent(edca->endBackoff(edca->numCategories() - 1));
        }
        EV_DEBUG << "deferring upper message transmission in " << fsm.getStateName() << " state\n";
        isInHandleWithFSM = false;
        return;
    }

    // Special case, is  endTimeout ACK and the radio state  is RECV, the system must wait until end reception (9.3.2.8 ACK procedure)
    if (msg == ackTimeout && radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING /* TODO: && useModulationParameters */&& fsm.getState() == WAITACK)
    {
        EV << "Re-schedule WAITACK timeout \n";
        scheduleAt(simTime() + controlFrameTxTime(LENGTH_ACK), ackTimeout);
        isInHandleWithFSM = false;
        return;
    }

    Ieee80211Frame *frame = dynamic_cast<Ieee80211Frame*>(msg);
    int frameType = frame ? frame->getType() : -1;
    logState();
    stateVector.record(fsm.getState());

    bool receptionError = false;
    if (frame && isLowerMessage(frame)) {
        lastReceiveFailed = receptionError = frame ? frame->hasBitError() : false;
        scheduleReservePeriod(frame);
    }

    // TODO: fix bug according to the message: [omnetpp] A possible bug in the Ieee80211's FSM.
    FSMA_Switch(fsm) {
        FSMA_State(IDLE) {
            FSMA_Enter(sendDownPendingRadioConfigMsg());
            /*
               if (fixFSM)
               {
               FSMA_Event_Transition(Data-Ready,
                                  // isUpperMessage(msg),
                                  isUpperMessage(msg) && backoffPeriod[currentAC] > 0,
                                  DEFER,
                //ASSERT(isInvalidBackoffPeriod() || backoffPeriod == 0);
                //invalidateBackoffPeriod();
               ASSERT(false);

               );
               FSMA_No_Event_Transition(Immediate-Data-Ready,
                                     //!transmissionQueue.empty(),
                !transmissionQueueEmpty(),
                                     DEFER,
               //  invalidateBackoffPeriod();
                ASSERT(backoff[currentAC]);

               );
               }
             */
            FSMA_Event_Transition(Data - Ready,
                    isUpperMessage(msg),
                    DEFER,
                    ASSERT(isInvalidBackoffPeriod() || edca->backoffPeriod() == SIMTIME_ZERO);
                    invalidateBackoffPeriod();
                    );
            FSMA_No_Event_Transition(Immediate - Data - Ready,
                    !edca->transmissionQueueEmpty(),
                    DEFER,
                    );
            FSMA_Event_Transition(Receive,
                    isLowerMessage(msg),
                    RECEIVE,
                    );
        }
        FSMA_State(DEFER) {
            FSMA_Enter(sendDownPendingRadioConfigMsg());
            FSMA_Event_Transition(Wait - AIFS,
                    isMediumStateChange(msg) && isMediumFree(),
                    WAITAIFS,
                    ;
                    );
            FSMA_No_Event_Transition(Immediate - Wait - AIFS,
                    isMediumFree() || (!isBackoffPending()),
                    WAITAIFS,
                    ;
                    );
            FSMA_Event_Transition(Receive,
                    isLowerMessage(msg),
                    RECEIVE,
                    ;
                    );
        }
        FSMA_State(WAITAIFS) {
            FSMA_Enter(scheduleAIFSPeriod());

            FSMA_Event_Transition(EDCAF - Do - Nothing,
                    isMsgAIFS(msg) && edca->transmissionQueue()->empty(),
                    WAITAIFS,
                    ASSERT(0 == 1);
                    ;
                    );
            FSMA_Event_Transition(Immediate - Transmit - RTS,
                    isMsgAIFS(msg) && !edca->transmissionQueue()->empty() && !isMulticast(edca->getCurrentTransmission())
                    && edca->getCurrentTransmission()->getByteLength() >= rtsThreshold && !edca->backoff(),
                    WAITCTS,
                    sendRTSFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    );
            FSMA_Event_Transition(Immediate - Transmit - Multicast,
                    isMsgAIFS(msg) && isMulticast(edca->getCurrentTransmission()) && !edca->backoff(),
                    WAITMULTICAST,
                    sendMulticastFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    );
            FSMA_Event_Transition(Immediate - Transmit - Data,
                    isMsgAIFS(msg) && !isMulticast(edca->getCurrentTransmission()) && !edca->backoff(),
                    WAITACK,
                    sendDataFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    );
            /*FSMA_Event_Transition(AIFS-Over,
                                  isMsgAIFS(msg) && backoff[currentAC],
                                  BACKOFF,
                if (isInvalidBackoffPeriod())
                    generateBackoffPeriod();
               );*/
            FSMA_Event_Transition(AIFS - Over,
                    isMsgAIFS(msg),
                    BACKOFF,
                    if (isInvalidBackoffPeriod())
                        generateBackoffPeriod();
                    );
            // end the difs and no other packet has been received
            FSMA_Event_Transition(DIFS - Over,
                    msg == endDIFS && edca->transmissionQueueEmpty(),
                    BACKOFF,
                    edca->setCurrentAc(edca->numCategories() - 1);
                    if (isInvalidBackoffPeriod())
                        generateBackoffPeriod();
                    );
            FSMA_Event_Transition(DIFS - Over,
                    msg == endDIFS,
                    BACKOFF,
                    for (int i = edca->numCategories() - 1; i >= 0; i--) {
                        if (!edca->transmissionQueue(i)->empty()) {
                            edca->setCurrentAc(i);
                        }
                    }
                    if (isInvalidBackoffPeriod())
                        generateBackoffPeriod();
                    );
            FSMA_Event_Transition(Busy,
                    isMediumStateChange(msg) && !isMediumFree(),
                    DEFER,
                    for (int i = 0; i < edca->numCategories(); i++) {
                        if (edca->endAIFS(i)->isScheduled())
                            edca->backoff(i) = true;
                    }
                    if (endDIFS->isScheduled())
                        edca->backoff(edca->numCategories() - 1) = true;
                    cancelAIFSPeriod();
                    );
            FSMA_No_Event_Transition(Immediate - Busy,
                    !isMediumFree(),
                    DEFER,
                    for (int i = 0; i < edca->numCategories(); i++) {
                        if (edca->endAIFS(i)->isScheduled())
                            edca->backoff(i) = true;
                    }
                    if (endDIFS->isScheduled())
                        edca->backoff(edca->numCategories() - 1) = true;
                    cancelAIFSPeriod();

                    );
            // radio state changes before we actually get the message, so this must be here
            FSMA_Event_Transition(Receive,
                    isLowerMessage(msg),
                    RECEIVE,
                    cancelAIFSPeriod();
                    ;
                    );
        }
        FSMA_State(BACKOFF) {
            FSMA_Enter(scheduleBackoffPeriod());
            if (edca->getCurrentTransmission()) {
                FSMA_Event_Transition(Transmit - RTS,
                        msg == edca->endBackoff() && !isMulticast(edca->getCurrentTransmission())
                        && edca->getCurrentTransmission()->getByteLength() >= rtsThreshold,
                        WAITCTS,
                        sendRTSFrame(edca->getCurrentTransmission());
                        edca->setOldCurrentAc(edca->getCurrentAc());
                        cancelAIFSPeriod();
                        decreaseBackoffPeriod();
                        cancelBackoffPeriod();
                        );
                FSMA_Event_Transition(Transmit - Multicast,
                        msg == edca->endBackoff() && isMulticast(edca->getCurrentTransmission()),
                        WAITMULTICAST,
                        sendMulticastFrame(edca->getCurrentTransmission());
                        edca->setOldCurrentAc(edca->getCurrentAc());
                        cancelAIFSPeriod();
                        decreaseBackoffPeriod();
                        cancelBackoffPeriod();
                        );
                FSMA_Event_Transition(Transmit - Data,
                        msg == edca->endBackoff() && !isMulticast(edca->getCurrentTransmission()),
                        WAITACK,
                        sendDataFrame(edca->getCurrentTransmission());
                        edca->setOldCurrentAc(edca->getCurrentAc());
                        cancelAIFSPeriod();
                        decreaseBackoffPeriod();
                        cancelBackoffPeriod();
                        );
            }
            FSMA_Event_Transition(AIFS - Over - backoff,
                    isMsgAIFS(msg) && edca->backoff(),
                    BACKOFF,
                    if (isInvalidBackoffPeriod())
                        generateBackoffPeriod();
                    );
            FSMA_Event_Transition(AIFS - Immediate - Transmit - RTS,
                    isMsgAIFS(msg) && !edca->transmissionQueue()->empty() && !isMulticast(edca->getCurrentTransmission())
                    && edca->getCurrentTransmission()->getByteLength() >= rtsThreshold && !edca->backoff(),
                    WAITCTS,
                    sendRTSFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    decreaseBackoffPeriod();
                    cancelBackoffPeriod();
                    );
            FSMA_Event_Transition(AIFS - Immediate - Transmit - Multicast,
                    isMsgAIFS(msg) && isMulticast(edca->getCurrentTransmission()) && !edca->backoff(),
                    WAITMULTICAST,
                    sendMulticastFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    decreaseBackoffPeriod();
                    cancelBackoffPeriod();
                    );
            FSMA_Event_Transition(AIFS - Immediate - Transmit - Data,
                    isMsgAIFS(msg) && !isMulticast(edca->getCurrentTransmission()) && !edca->backoff(),
                    WAITACK,
                    sendDataFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    cancelAIFSPeriod();
                    decreaseBackoffPeriod();
                    cancelBackoffPeriod();
                    );
            FSMA_Event_Transition(Backoff - Idle,
                    edca->isBackoffMsg(msg) && edca->transmissionQueueEmpty(),
                    IDLE,
                    resetStateVariables();
                    );
            FSMA_Event_Transition(Backoff - Busy,
                    isMediumStateChange(msg) && !isMediumFree(),
                    DEFER,
                    cancelAIFSPeriod();
                    decreaseBackoffPeriod();
                    cancelBackoffPeriod();
                    );
        }
        FSMA_State(WAITACK) {
            FSMA_Enter(scheduleAckTimeoutPeriod(edca->getCurrentTransmission()));
#ifndef DISABLEERRORACK
            FSMA_Event_Transition(Reception - ACK - failed,
                    isLowerMessage(msg) && receptionError && edca->retryCounter(edca->getOldCurrentAc()) == transmissionLimit - 1,
                    IDLE,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    cancelTimeoutPeriod();
                    giveUpCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
            FSMA_Event_Transition(Reception - ACK - error,
                    isLowerMessage(msg) && receptionError,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    cancelTimeoutPeriod();
                    retryCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
#endif // ifndef DISABLEERRORACK
            FSMA_Event_Transition(Receive - ACK - TXOP - Empty,
                    isLowerMessage(msg) && isForUs(frame) && frameType == ST_ACK && txop && edca->transmissionQueue(edca->getOldCurrentAc())->size() == 1,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    if (edca->retryCounter() == 0)
                        edca->numSentWithoutRetry()++;
                    edca->numSent()++;
                    fr = edca->getCurrentTransmission();
                    numBits += fr->getBitLength();
                    edca->bits() += fr->getBitLength();
                    edca->macDelay()->record(simTime() - fr->getMACArrive());
                    if (edca->maxJitter() == SIMTIME_ZERO || edca->maxJitter() < (simTime() - fr->getMACArrive()))
                        edca->maxJitter() = simTime() - fr->getMACArrive();
                    if (edca->minJitter() == SIMTIME_ZERO || edca->minJitter() > (simTime() - fr->getMACArrive()))
                        edca->minJitter() = simTime() - fr->getMACArrive();
                    EV_DEBUG << "record macDelay AC" << edca->getCurrentAc() << " value " << simTime() - fr->getMACArrive() << endl;
                    numSentTXOP++;
                    cancelTimeoutPeriod();
                    finishCurrentTransmission();
                    resetCurrentBackOff();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
            FSMA_Event_Transition(Receive - ACK - TXOP,
                    isLowerMessage(msg) && isForUs(frame) && frameType == ST_ACK && txop,
                    WAITSIFS,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    if (edca->retryCounter() == 0)
                        edca->numSentWithoutRetry()++;
                    edca->numSent()++;
                    fr = edca->getCurrentTransmission();
                    numBits += fr->getBitLength();
                    edca->bits() += fr->getBitLength();

                    edca->macDelay()->record(simTime() - fr->getMACArrive());
                    if (edca->maxJitter() == SIMTIME_ZERO || edca->maxJitter() < (simTime() - fr->getMACArrive()))
                        edca->maxJitter() = simTime() - fr->getMACArrive();
                    if (edca->minJitter() == SIMTIME_ZERO || edca->minJitter() > (simTime() - fr->getMACArrive()))
                        edca->minJitter() = simTime() - fr->getMACArrive();
                    EV_DEBUG << "record macDelay AC" << edca->getCurrentAc() << " value " << simTime() - fr->getMACArrive() << endl;
                    numSentTXOP++;
                    cancelTimeoutPeriod();
                    finishCurrentTransmission();
                    );
/*
            FSMA_Event_Transition(Receive-ACK,
                                  isLowerMessage(msg) && isForUs(frame) && frameType == ST_ACK,
                                  IDLE,
                                  currentAC=edca->getOldcurrentAc();
                                  if (retryCounter[currentAC] == 0) numSentWithoutRetry[currentAC]++;
                                  numSent[currentAC]++;
                                  fr=edca->getCurrentTransmission();
                                  numBites += fr->getBitLength();
                                  bits[currentAC] += fr->getBitLength();

                                  macDelay[currentAC].record(simTime() - fr->getMACArrive());
                                  if (maxjitter[currentAC] == 0 || maxjitter[currentAC] < (simTime() - fr->getMACArrive())) maxjitter[currentAC]=simTime() - fr->getMACArrive();
                                      if (minjitter[currentAC] == 0 || minjitter[currentAC] > (simTime() - fr->getMACArrive())) minjitter[currentAC]=simTime() - fr->getMACArrive();
                                          EV_DEBUG << "record macDelay AC" << currentAC << " value " << simTime() - fr->getMACArrive() <<endl;

                                          cancelTimeoutPeriod();
                                          finishCurrentTransmission();
                                         );

 */
            /*Ieee 802.11 2007 9.9.1.2 EDCA TXOPs*/
            FSMA_Event_Transition(Receive - ACK,
                    isLowerMessage(msg) && isForUs(frame) && frameType == ST_ACK,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    if (edca->retryCounter() == 0)
                        edca->numSentWithoutRetry()++;
                    edca->numSent()++;
                    fr = edca->getCurrentTransmission();
                    numBits += fr->getBitLength();
                    edca->bits() += fr->getBitLength();

                    edca->macDelay()->record(simTime() - fr->getMACArrive());
                    if (edca->maxJitter() == SIMTIME_ZERO || edca->maxJitter() < (simTime() - fr->getMACArrive()))
                        edca->maxJitter() = simTime() - fr->getMACArrive();
                    if (edca->minJitter() == SIMTIME_ZERO || edca->minJitter() > (simTime() - fr->getMACArrive()))
                        edca->minJitter() = simTime() - fr->getMACArrive();
                    EV_DEBUG << "record macDelay AC" << edca->getCurrentAc() << " value " << simTime() - fr->getMACArrive() << endl;
                    cancelTimeoutPeriod();
                    finishCurrentTransmission();
                    resetCurrentBackOff();
                    );
            FSMA_Event_Transition(Transmit - Data - Failed,
                    msg == ackTimeout && edca->retryCounter(edca->getOldCurrentAc()) == transmissionLimit - 1,
                    IDLE,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    giveUpCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
            FSMA_Event_Transition(Receive - ACK - Timeout,
                    msg == ackTimeout,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    retryCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
            FSMA_Event_Transition(Interrupted - ACK - Failure,
                    isLowerMessage(msg) && edca->retryCounter(edca->getOldCurrentAc()) == transmissionLimit - 1,
                    RECEIVE,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    cancelTimeoutPeriod();
                    giveUpCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
            FSMA_Event_Transition(Retry - Interrupted - ACK,
                    isLowerMessage(msg),
                    RECEIVE,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    cancelTimeoutPeriod();
                    retryCurrentTransmission();
                    txop = false;
                    if (endTXOP->isScheduled())
                        cancelEvent(endTXOP);
                    );
        }
        // wait until multicast is sent
        FSMA_State(WAITMULTICAST) {
            FSMA_Enter(scheduleMulticastTimeoutPeriod(edca->getCurrentTransmission()));
            /*
                        FSMA_Event_Transition(Transmit-Multicast,
                                              msg == endTimeout,
                                              IDLE,
                            currentAC=edca->getOldcurrentAc();
                            finishCurrentTransmission();
                            numSentMulticast++;
                        );
             */
            ///changed
            FSMA_Event_Transition(Transmit - Multicast,
                    msg == ackTimeout,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    fr = edca->getCurrentTransmission();
                    numBits += fr->getBitLength();
                    edca->bits() += fr->getBitLength();
                    finishCurrentTransmission();
                    numSentMulticast++;
                    resetCurrentBackOff();
                    );
        }
        // accoriding to 9.2.5.7 CTS procedure
        FSMA_State(WAITCTS) {
            FSMA_Enter(scheduleCTSTimeoutPeriod());
#ifndef DISABLEERRORACK
            FSMA_Event_Transition(Reception - CTS - Failed,
                    isLowerMessage(msg) && receptionError && edca->retryCounter(edca->getOldCurrentAc()) == transmissionLimit - 1,
                    IDLE,
                    cancelTimeoutPeriod();
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    giveUpCurrentTransmission();
                    );
            FSMA_Event_Transition(Reception - CTS - error,
                    isLowerMessage(msg) && receptionError,
                    DEFER,
                    cancelTimeoutPeriod();
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    retryCurrentTransmission();
                    );
#endif // ifndef DISABLEERRORACK
            FSMA_Event_Transition(Receive - CTS,
                    isLowerMessage(msg) && isForUs(frame) && frameType == ST_CTS,
                    WAITSIFS,
                    cancelTimeoutPeriod();
                    );
            FSMA_Event_Transition(Transmit - RTS - Failed,
                    msg == ackTimeout && edca->retryCounter(edca->getOldCurrentAc()) == transmissionLimit - 1,
                    IDLE,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    giveUpCurrentTransmission();
                    );
            FSMA_Event_Transition(Receive - CTS - Timeout,
                    msg == ackTimeout,
                    DEFER,
                    edca->setCurrentAc(edca->getOldCurrentAc());
                    retryCurrentTransmission();
                    );
        }
        FSMA_State(WAITSIFS) {
            FSMA_Enter(scheduleSIFSPeriod(frame));
            FSMA_Event_Transition(Transmit - Data - TXOP,
                    msg == endSIFS && getFrameReceivedBeforeSIFS()->getType() == ST_ACK,
                    WAITACK,
                    sendDataFrame(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    );
            FSMA_Event_Transition(Transmit - CTS,
                    msg == endSIFS && getFrameReceivedBeforeSIFS()->getType() == ST_RTS,
                    IDLE,
                    sendCTSFrameOnEndSIFS();
                    finishReception();
                    );
            FSMA_Event_Transition(Transmit - DATA,
                    msg == endSIFS && getFrameReceivedBeforeSIFS()->getType() == ST_CTS,
                    WAITACK,
                    sendDataFrameOnEndSIFS(edca->getCurrentTransmission());
                    edca->setOldCurrentAc(edca->getCurrentAc());
                    );
            FSMA_Event_Transition(Transmit - ACK,
                    msg == endSIFS && isDataOrMgmtFrame(getFrameReceivedBeforeSIFS()),
                    IDLE,
                    sendACKFrameOnEndSIFS();
                    finishReception();
                    );
        }
        // this is not a real state
        FSMA_State(RECEIVE) {
            FSMA_No_Event_Transition(Immediate - Receive - Error,
                    isLowerMessage(msg) && receptionError,
                    IDLE,
                    EV_WARN << "received frame contains bit errors or collision, next wait period is EIFS\n";
                    numCollision++;
                    finishReception();
                    );
            FSMA_No_Event_Transition(Immediate - Receive - Multicast,
                    isLowerMessage(msg) && isMulticast(frame) && !isSentByUs(frame) && isDataOrMgmtFrame(frame),
                    IDLE,
                    sendUp(frame);
                    numReceivedMulticast++;
                    finishReception();
                    );
            FSMA_No_Event_Transition(Immediate - Receive - Data,
                    isLowerMessage(msg) && isForUs(frame) && isDataOrMgmtFrame(frame),
                    WAITSIFS,
                    sendUp(frame);
                    numReceived++;
                    );
            FSMA_No_Event_Transition(Immediate - Receive - RTS,
                    isLowerMessage(msg) && isForUs(frame) && frameType == ST_RTS,
                    WAITSIFS,
                    );
            FSMA_No_Event_Transition(Immediate - Receive - Other - backtobackoff,
                    isLowerMessage(msg) && isBackoffPending(),    //(backoff[0] || backoff[1] || backoff[2] || backoff[3]),
                    DEFER,
                    );

            FSMA_No_Event_Transition(Immediate - Promiscuous - Data,
                    isLowerMessage(msg) && !isForUs(frame) && isDataOrMgmtFrame(frame),
                    IDLE,
                    promiscousFrame(frame);
                    finishReception();
                    numReceivedOther++;
                    );
            FSMA_No_Event_Transition(Immediate - Receive - Other,
                    isLowerMessage(msg),
                    IDLE,
                    finishReception();
                    numReceivedOther++;
                    );
        }
    }
    EV_TRACE << "leaving handleWithFSM\n\t";
    logState();
    stateVector.record(fsm.getState());
    if (simTime() - last > 0.1) {
        for (int i = 0; i < edca->numCategories(); i++) {
            edca->throughput(i)->record(edca->bits(i) / (simTime() - last));
            edca->bits(i) = 0;
            if (edca->maxJitter(i) > SIMTIME_ZERO && edca->minJitter(i) > SIMTIME_ZERO) {
                edca->jitter(i)->record(edca->maxJitter(i) - edca->minJitter(i));
                edca->maxJitter(i) = SIMTIME_ZERO;
                edca->minJitter(i) = SIMTIME_ZERO;
            }
        }
        last = simTime();
    }
    isInHandleWithFSM = false;
}

void Ieee80211Mac::finishReception()
{
    if (edca->getCurrentTransmission()) {
        edca->backoff() = true;
    }
    else {
        resetStateVariables();
    }
}

/****************************************************************
 * Timing functions.
 */
simtime_t Ieee80211Mac::getSIFS()
{
    return dataFrameMode->getSifsTime();
}

simtime_t Ieee80211Mac::getSlotTime()
{
// TODO:   return aCCATime() + aRxTxTurnaroundTime + aAirPropagationTime() + aMACProcessingDelay();
    return dataFrameMode->getSlotTime();
}

simtime_t Ieee80211Mac::getPIFS()
{
    return getSIFS() + getSlotTime();
}

simtime_t Ieee80211Mac::getDIFS(int category)
{
    if (category < 0 || category > (edca->numCategories() - 1)) {
        int index = edca->numCategories() - 1;
        if (index < 0)
            index = 0;
        return getSIFS() + ((double)edca->AIFSN(index) * getSlotTime());
    }
    else {
        return getSIFS() + ((double)edca->AIFSN(category)) * getSlotTime();
    }
}

simtime_t Ieee80211Mac::getAIFS(int AccessCategory)
{
    return edca->AIFSN(AccessCategory) * getSlotTime() + getSIFS();
}

simtime_t Ieee80211Mac::getEIFS()
{
    // EIFS = aSIFSTime + DIFS + ACKTxTime, where ACKTxTime is the time expressed in
    // microseconds required to transmit an ACK frame, including preamble, PLCP header
    // and any additional PHY dependent information, at the lowest PHY mandatory rate.
    return getSIFS() + getDIFS() + controlFrameTxTime(LENGTH_ACK); // TODO: Is controlFrameMode always the lowest PHY mandatory mode?
}

simtime_t Ieee80211Mac::computeBackoffPeriod(Ieee80211Frame *msg, int r)
{
    int cw;

    EV_DEBUG << "generating backoff slot number for retry: " << r << endl;
    if (msg && isMulticast(msg))
        cw = cwMinMulticast;
    else {
        ASSERT(0 <= r && r < transmissionLimit);

        cw = (edca->cwMin() + 1) * (1 << r) - 1;

        if (cw > edca->cwMax())
            cw = edca->cwMax();
    }

    int c = intrand(cw + 1);

    EV_DEBUG << "generated backoff slot number: " << c << " , cw: " << cw << " ,cwMin:cwMax = " << edca->cwMin() << ":" << edca->cwMax() << endl;

    return ((double)c) * getSlotTime();
}

/****************************************************************
 * Timer functions.
 */
void Ieee80211Mac::scheduleSIFSPeriod(Ieee80211Frame *frame)
{
    EV_DEBUG << "scheduling SIFS period\n";
    endSIFS->setContextPointer(frame->dup());
    scheduleAt(simTime() + getSIFS(), endSIFS);
}

void Ieee80211Mac::scheduleDIFSPeriod()
{
    if (lastReceiveFailed) {
        EV_DEBUG << "reception of last frame failed, scheduling EIFS period\n";
        scheduleAt(simTime() + getEIFS(), endDIFS);
    }
    else {
        EV_DEBUG << "scheduling DIFS period\n";
        scheduleAt(simTime() + getDIFS(), endDIFS);
    }
}

void Ieee80211Mac::cancelDIFSPeriod()
{
    EV_DEBUG << "canceling DIFS period\n";
    cancelEvent(endDIFS);
}

void Ieee80211Mac::scheduleAIFSPeriod()
{
    bool schedule = false;
    for (int i = 0; i < edca->numCategories(); i++) {
        if (!edca->endAIFS(i)->isScheduled() && !edca->transmissionQueue(i)->empty()) {
            if (lastReceiveFailed) {
                EV_DEBUG << "reception of last frame failed, scheduling EIFS-DIFS+AIFS period (" << i << ")\n";
                scheduleAt(simTime() + getEIFS() - getDIFS() + getAIFS(i), edca->endAIFS(i));
            }
            else {
                EV_DEBUG << "scheduling AIFS period (" << i << ")\n";
                scheduleAt(simTime() + getAIFS(i), edca->endAIFS(i));
            }
        }
        if (edca->endAIFS(i)->isScheduled())
            schedule = true;
    }
    if (!schedule && !endDIFS->isScheduled()) {
        // schedule default DIFS
        edca->setCurrentAc(edca->numCategories() - 1);
        scheduleDIFSPeriod();
    }
}

void Ieee80211Mac::rescheduleAIFSPeriod(int AccessCategory)
{
    ASSERT(1);
    EV_DEBUG << "rescheduling AIFS[" << AccessCategory << "]\n";
    cancelEvent(edca->endAIFS(AccessCategory));
    scheduleAt(simTime() + getAIFS(AccessCategory), edca->endAIFS(AccessCategory));
}

void Ieee80211Mac::cancelAIFSPeriod()
{
    EV_DEBUG << "canceling AIFS period\n";
    for (int i = 0; i < edca->numCategories(); i++)
        cancelEvent(edca->endAIFS(i));
    cancelEvent(endDIFS);
}

//XXXvoid Ieee80211Mac::checkInternalColision()
//{
//  EV_DEBUG << "We obtain endAIFS, so we have to check if there
//}

void Ieee80211Mac::scheduleAckTimeoutPeriod(Ieee80211DataOrMgmtFrame *frameToSend)
{
//    After transmitting an MPDU that requires an ACK frame as a response (see Annex G), the STA shall wait for an
//    ACKTimeout interval, with a value of aSIFSTime + aSlotTime + aPHY-RX-START-Delay, starting at the
//    PHY-TXEND.confirm primitive., p. 834
//    TransmissionRequest *trq = dynamic_cast<TransmissionRequest *>(frameToSend->getControlInfo());
//    ???
//    if (trq) {
//        bitRate = trq->getBitrate().get();
//        if (bitRate == 0)
//            bitRate = dataFrameMode->getDataMode()->getNetBitrate().get();
//    }
    if (!ackTimeout->isScheduled()) { // TODO
        EV_DEBUG << "Scheduling ACK timeout period" << endl;
        simtime_t duration = dataFrameMode->getDuration(frameToSend->getBitLength());
        simtime_t ackTimeoutInterval = duration + getSIFS() + getSlotTime() + dataFrameMode->getPhyRxStartDelay();
        EV_DEBUG << "ACK timeout = " << ackTimeout << " us" << endl;
        scheduleAt(simTime() + ackTimeoutInterval, ackTimeout);
    }
}

void Ieee80211Mac::scheduleMulticastTimeoutPeriod(Ieee80211DataOrMgmtFrame *frameToSend)
{
    if (!ackTimeout->isScheduled()) {
        EV_DEBUG << "scheduling multicast timeout period\n";
        scheduleAt(simTime() + computeFrameDuration(frameToSend), ackTimeout);
    }
}

void Ieee80211Mac::cancelTimeoutPeriod()
{
    EV_DEBUG << "canceling timeout period\n";
    cancelEvent(ackTimeout);
}

void Ieee80211Mac::scheduleCTSTimeoutPeriod()
{
    if (!ackTimeout->isScheduled()) {
        EV_DEBUG << "scheduling CTS timeout period\n";
        scheduleAt(simTime() + controlFrameTxTime(LENGTH_RTS) + getSIFS()
                   + controlFrameTxTime(LENGTH_CTS) + MAX_PROPAGATION_DELAY * 2, ackTimeout);
    }
}

void Ieee80211Mac::scheduleReservePeriod(Ieee80211Frame *frame)
{
    simtime_t reserve = frame->getDuration();

    // see spec. 7.1.3.2
    if (!isForUs(frame) && reserve != 0 && reserve < 32768) {
        if (endReserve->isScheduled()) {
            simtime_t oldReserve = endReserve->getArrivalTime() - simTime();

            if (oldReserve > reserve)
                return;

            reserve = std::max(reserve, oldReserve);
            cancelEvent(endReserve);
        }
        else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE) {
            // NAV: the channel just became virtually busy according to the spec
            scheduleAt(simTime(), mediumStateChange);
        }

        EV_DEBUG << "scheduling reserve period for: " << reserve << endl;

        ASSERT(reserve > 0);

        nav = true;
        scheduleAt(simTime() + reserve, endReserve);
    }
}

void Ieee80211Mac::invalidateBackoffPeriod()
{
    edca->backoffPeriod() = -1;
}

bool Ieee80211Mac::isInvalidBackoffPeriod()
{
    return edca->backoffPeriod() == -1;
}

void Ieee80211Mac::generateBackoffPeriod()
{
    edca->backoffPeriod() = computeBackoffPeriod(edca->getCurrentTransmission(), edca->retryCounter());
    ASSERT(edca->backoffPeriod() >= SIMTIME_ZERO);
    EV_DEBUG << "backoff period set to " << edca->backoffPeriod() << endl;
}

void Ieee80211Mac::decreaseBackoffPeriod()
{
    // see spec 9.9.1.5
    // decrase for every EDCAF
    // cancel event endBackoff after decrease or we don't know which endBackoff is scheduled
    for (int i = 0; i < edca->numCategories(); i++) {
        if (edca->backoff(i) && edca->endBackoff(i)->isScheduled()) {
            EV_DEBUG << "old backoff[" << i << "] is " << edca->backoffPeriod(i) << ", sim time is " << simTime()
                     << ", endbackoff sending period is " << edca->endBackoff(i)->getSendingTime() << endl;
            simtime_t elapsedBackoffTime = simTime() - edca->endBackoff(i)->getSendingTime();
            edca->backoffPeriod(i) -= ((int)(elapsedBackoffTime / getSlotTime())) * getSlotTime();
            EV_DEBUG << "actual backoff[" << i << "] is " << edca->backoffPeriod(i) << ", elapsed is " << elapsedBackoffTime << endl;
            ASSERT(edca->backoffPeriod(i) >= SIMTIME_ZERO);
            EV_DEBUG << "backoff[" << i << "] period decreased to " << edca->backoffPeriod(i) << endl;
        }
    }
}

void Ieee80211Mac::scheduleBackoffPeriod()
{
    EV_DEBUG << "scheduling backoff period\n";
    scheduleAt(simTime() + edca->backoffPeriod(), edca->endBackoff());
}

void Ieee80211Mac::cancelBackoffPeriod()
{
    EV_DEBUG << "cancelling Backoff period - only if some is scheduled\n";
    for (int i = 0; i < edca->numCategories(); i++)
        cancelEvent(edca->endBackoff(i));
}

/****************************************************************
 * Frame sender functions.
 */
void Ieee80211Mac::sendACKFrameOnEndSIFS()
{
    Ieee80211Frame *frameToACK = (Ieee80211Frame *)endSIFS->getContextPointer();
    endSIFS->setContextPointer(nullptr);
    sendACKFrame(check_and_cast<Ieee80211DataOrMgmtFrame *>(frameToACK));
    delete frameToACK;
}

void Ieee80211Mac::sendACKFrame(Ieee80211DataOrMgmtFrame *frameToACK)
{
    EV_INFO << "sending ACK frame\n";
    numAckSend++;
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(setControlBitrate(buildACKFrame(frameToACK)));
}

void Ieee80211Mac::sendDataFrameOnEndSIFS(Ieee80211DataOrMgmtFrame *frameToSend)
{
    Ieee80211Frame *ctsFrame = (Ieee80211Frame *)endSIFS->getContextPointer();
    endSIFS->setContextPointer(nullptr);
    sendDataFrame(frameToSend);
    delete ctsFrame;
}

void Ieee80211Mac::sendDataFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    simtime_t t = 0, time = 0;
    int count = 0;
    auto frame = edca->transmissionQueue()->begin();
    ASSERT(*frame == frameToSend);
    if (!txop && edca->TXOP() > 0 && edca->transmissionQueue()->size() >= 2) {
        //we start packet burst within TXOP time period
        txop = true;

        for (frame = edca->transmissionQueue()->begin(); frame != edca->transmissionQueue()->end(); ++frame) {
            count++;
            t = computeFrameDuration(*frame) + 2 * getSIFS() + controlFrameTxTime(LENGTH_ACK);
            EV_DEBUG << "t is " << t << endl;
            if (edca->TXOP() > time + t) {
                time += t;
                EV_DEBUG << "adding t\n";
            }
            else {
                break;
            }
        }
        //to be sure we get endTXOP earlier then receive ACK and we have to minus SIFS time from first packet
        time -= getSIFS() / 2 + getSIFS();
        EV_DEBUG << "scheduling TXOP for AC" << edca->getCurrentAc() << ", duration is " << time << ", count is " << count << endl;
        scheduleAt(simTime() + time, endTXOP);
    }
    EV_INFO << "sending Data frame\n";
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(buildDataFrame(dynamic_cast<Ieee80211DataOrMgmtFrame *>(setBitrateFrame(frameToSend))));
}

void Ieee80211Mac::sendRTSFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    EV_INFO << "sending RTS frame\n";
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(setControlBitrate(buildRTSFrame(frameToSend)));
}

void Ieee80211Mac::sendMulticastFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    EV_INFO << "sending Multicast frame\n";
    if (frameToSend->getControlInfo())
        delete frameToSend->removeControlInfo();
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(buildDataFrame(dynamic_cast<Ieee80211DataOrMgmtFrame *>(setBasicBitrate(frameToSend))));
}

void Ieee80211Mac::sendCTSFrameOnEndSIFS()
{
    Ieee80211Frame *rtsFrame = (Ieee80211Frame *)endSIFS->getContextPointer();
    endSIFS->setContextPointer(nullptr);
    sendCTSFrame(check_and_cast<Ieee80211RTSFrame *>(rtsFrame));
    delete rtsFrame;
}

void Ieee80211Mac::sendCTSFrame(Ieee80211RTSFrame *rtsFrame)
{
    EV_INFO << "sending CTS frame\n";
    configureRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
    sendDown(setControlBitrate(buildCTSFrame(rtsFrame)));
}

/****************************************************************
 * Frame builder functions.
 */
Ieee80211DataOrMgmtFrame *Ieee80211Mac::buildDataFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    Ieee80211DataOrMgmtFrame *frame = (Ieee80211DataOrMgmtFrame *)frameToSend->dup();

    if (frameToSend->getControlInfo() != nullptr) {
        cObject *ctr = frameToSend->getControlInfo();
        TransmissionRequest *ctrl = dynamic_cast<TransmissionRequest *>(ctr);
        if (ctrl == nullptr)
            throw cRuntimeError("control info is not TransmissionRequest but %s", ctr->getClassName());
        frame->setControlInfo(ctrl->dup());
    }
    if (isMulticast(frameToSend))
        frame->setDuration(0);
    else if (!frameToSend->getMoreFragments()) {
        if (txop && edca->transmissionQueue()->size() > 1) {
            // ++ operation is safe because txop is true
            auto nextframeToSend = edca->transmissionQueue()->begin();
            nextframeToSend++;
            ASSERT(edca->transmissionQueue()->end() != nextframeToSend);
            double bitRate = dataFrameMode->getDataMode()->getNetBitrate().get();
            int size = (*nextframeToSend)->getBitLength();
            TransmissionRequest *trRq = dynamic_cast<TransmissionRequest *>(edca->transmissionQueue()->front()->getControlInfo());
            if (trRq) {
                bitRate = trRq->getBitrate().get();
                if (bitRate == 0)
                    bitRate = dataFrameMode->getDataMode()->getNetBitrate().get();
            }
            frame->setDuration(3 * getSIFS() + 2 * controlFrameTxTime(LENGTH_ACK) + computeFrameDuration(size, bitRate));
        }
        else
            frame->setDuration(getSIFS() + controlFrameTxTime(LENGTH_ACK));
    }
    else
        // FIXME: shouldn't we use the next frame to be sent?
        frame->setDuration(3 * getSIFS() + 2 * controlFrameTxTime(LENGTH_ACK) + computeFrameDuration(frameToSend));

    return frame;
}

Ieee80211ACKFrame *Ieee80211Mac::buildACKFrame(Ieee80211DataOrMgmtFrame *frameToACK)
{
    Ieee80211ACKFrame *frame = new Ieee80211ACKFrame("wlan-ack");
    frame->setReceiverAddress(frameToACK->getTransmitterAddress());

    if (!frameToACK->getMoreFragments())
        frame->setDuration(0);
    else
        frame->setDuration(frameToACK->getDuration() - getSIFS() - controlFrameTxTime(LENGTH_ACK));

    return frame;
}

Ieee80211RTSFrame *Ieee80211Mac::buildRTSFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    Ieee80211RTSFrame *frame = new Ieee80211RTSFrame("wlan-rts");
    frame->setTransmitterAddress(address);
    frame->setReceiverAddress(frameToSend->getReceiverAddress());
    frame->setDuration(3 * getSIFS() + controlFrameTxTime(LENGTH_CTS)
            + computeFrameDuration(frameToSend)
            + controlFrameTxTime(LENGTH_ACK));

    return frame;
}

Ieee80211CTSFrame *Ieee80211Mac::buildCTSFrame(Ieee80211RTSFrame *rtsFrame)
{
    Ieee80211CTSFrame *frame = new Ieee80211CTSFrame("wlan-cts");
    frame->setReceiverAddress(rtsFrame->getTransmitterAddress());
    frame->setDuration(rtsFrame->getDuration() - getSIFS() - controlFrameTxTime(LENGTH_CTS));

    return frame;
}

Ieee80211DataOrMgmtFrame *Ieee80211Mac::buildMulticastFrame(Ieee80211DataOrMgmtFrame *frameToSend)
{
    Ieee80211DataOrMgmtFrame *frame = (Ieee80211DataOrMgmtFrame *)frameToSend->dup();

    TransmissionRequest *oldTransmissionRequest = dynamic_cast<TransmissionRequest *>(frameToSend->getControlInfo());
    if (oldTransmissionRequest) {
        EV_DEBUG << "Per frame1 params" << endl;
        TransmissionRequest *newTransmissionRequest = new TransmissionRequest();
        *newTransmissionRequest = *oldTransmissionRequest;
        frame->setControlInfo(newTransmissionRequest);
    }

    frame->setDuration(0);
    return frame;
}

Ieee80211Frame *Ieee80211Mac::setBasicBitrate(Ieee80211Frame *frame)
{
    ASSERT(frame->getControlInfo() == nullptr);
    TransmissionRequest *ctrl = new TransmissionRequest();
    ctrl->setBitrate(bps(basicFrameMode->getDataMode()->getNetBitrate()));
    frame->setControlInfo(ctrl);
    return frame;
}

Ieee80211Frame *Ieee80211Mac::setControlBitrate(Ieee80211Frame *frame)
{
    ASSERT(frame->getControlInfo()==nullptr);
    TransmissionRequest *ctrl = new TransmissionRequest();
    ctrl->setBitrate((bps)controlFrameMode->getDataMode()->getNetBitrate());
    frame->setControlInfo(ctrl);
    return frame;
}

Ieee80211Frame *Ieee80211Mac::setBitrateFrame(Ieee80211Frame *frame)
{
    if (autoRate == nullptr /* TODO: && autoRatePlugin->isForceBitRate() == false*/) {
        if (frame->getControlInfo())
            delete frame->removeControlInfo();
        return frame;
    }
    TransmissionRequest *ctrl = nullptr;
    if (frame->getControlInfo() == nullptr) {
        ctrl = new TransmissionRequest();
        frame->setControlInfo(ctrl);
    }
    else
        ctrl = dynamic_cast<TransmissionRequest *>(frame->getControlInfo());
    if (ctrl)
        ctrl->setBitrate(bps(getBitrate()));
    return frame;
}

/****************************************************************
 * Helper functions.
 */
void Ieee80211Mac::finishCurrentTransmission()
{
    popTransmissionQueue();
    resetStateVariables();
}

void Ieee80211Mac::giveUpCurrentTransmission()
{
    Ieee80211DataOrMgmtFrame *temp = (Ieee80211DataOrMgmtFrame *)edca->transmissionQueue()->front();
    emit(NF_LINK_BREAK, temp);
    popTransmissionQueue();
    resetStateVariables();
    edca->numGivenUp()++;
}

void Ieee80211Mac::retryCurrentTransmission()
{
    ASSERT(edca->retryCounter() < transmissionLimit - 1);
    edca->getCurrentTransmission()->setRetry(true);
    edca->retryCounter()++;
    if (autoRate)
        autoRate->reportDataFailed(modeSet, dataFrameMode, edca->retryCounter(), needNormalFallback());
    edca->numRetry()++;
    edca->backoff() = true;
    generateBackoffPeriod();
}

void Ieee80211Mac::sendDownPendingRadioConfigMsg()
{
    if (pendingRadioConfigMsg != nullptr) {
        sendDown(pendingRadioConfigMsg);
        pendingRadioConfigMsg = nullptr;
    }
}

void Ieee80211Mac::setMode(Mode mode)
{
    if (mode == PCF)
        throw cRuntimeError("PCF mode not yet supported");

    this->mode = mode;
}

void Ieee80211Mac::resetStateVariables()
{
    edca->backoffPeriod() = SIMTIME_ZERO;
    edca->retryCounter() = 0;
    if (autoRate)
        autoRate->reportDataOk(modeSet, dataFrameMode);
    if (!edca->transmissionQueue()->empty()) {
        edca->backoff() = true;
        edca->getCurrentTransmission()->setRetry(false);
    }
    else {
        edca->backoff() = false;
    }
}

bool Ieee80211Mac::isMediumStateChange(cMessage *msg)
{
    return msg == mediumStateChange || (msg == endReserve && radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE);
}

bool Ieee80211Mac::isMediumFree()
{
    return !endReserve->isScheduled() && radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE;
}

bool Ieee80211Mac::isMulticast(Ieee80211Frame *frame)
{
    return frame && frame->getReceiverAddress().isMulticast();
}

bool Ieee80211Mac::isForUs(Ieee80211Frame *frame)
{
    return frame && frame->getReceiverAddress() == address;
}

bool Ieee80211Mac::isSentByUs(Ieee80211Frame *frame)
{
    if (dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame)) {
        //EV_DEBUG << "ad3 "<<((Ieee80211DataOrMgmtFrame *)frame)->getAddress3();
        //EV_DEBUG << "myad "<<address<<endl;
        if (((Ieee80211DataOrMgmtFrame *)frame)->getAddress3() == address) //received frame sent by us
            return 1;
    }
    else
        EV_ERROR << "Cast failed" << endl; // WTF? (levy)

    return 0;
}

bool Ieee80211Mac::isDataOrMgmtFrame(Ieee80211Frame *frame)
{
    return dynamic_cast<Ieee80211DataOrMgmtFrame *>(frame);
}

bool Ieee80211Mac::isMsgAIFS(cMessage *msg)
{
    for (int i = 0; i < edca->numCategories(); i++)
        if (msg == edca->endAIFS(i))
            return true;

    return false;
}

Ieee80211Frame *Ieee80211Mac::getFrameReceivedBeforeSIFS()
{
    return (Ieee80211Frame *)endSIFS->getContextPointer();
}

void Ieee80211Mac::popTransmissionQueue()
{
    EV_DEBUG << "dropping frame from transmission queue\n";
    Ieee80211Frame *temp = dynamic_cast<Ieee80211Frame *>(edca->transmissionQueue()->front());
    ASSERT(!edca->transmissionQueue()->empty());
    edca->transmissionQueue()->pop_front();
    if (queueModule) {
        if (edca->numCategories() == 1) {
            // the module are continuously asking for packets
            EV_DEBUG << "requesting another frame from queue module\n";
            queueModule->requestPacket();
        }
        else if (edca->numCategories() > 1 && (int)edca->transmissionQueueSize() == maxQueueSize - 1) {
            // Now exist a empty frame space
            // the module are continuously asking for packets
            EV_DEBUG << "requesting another frame from queue module\n";
            queueModule->requestPacket();
        }
    }
    delete temp;
}

double Ieee80211Mac::computeFrameDuration(Ieee80211Frame *msg)
{
    TransmissionRequest *ctrl;
    double duration;
    EV_DEBUG << *msg;
    ctrl = dynamic_cast<TransmissionRequest *>(msg->removeControlInfo());
    if (ctrl) {
        EV_DEBUG << "Per frame2 params bitrate " << ctrl->getBitrate().get() / 1e6 << "Mb" << endl;
        duration = computeFrameDuration(msg->getBitLength(), ctrl->getBitrate().get());
        delete ctrl;
        return duration;
    }
    else
        return computeFrameDuration(msg->getBitLength(), dataFrameMode->getDataMode()->getNetBitrate().get());
}

double Ieee80211Mac::computeFrameDuration(int bits, double bitrate)
{
    double duration;
    const IIeee80211Mode *modType = modeSet->getMode(bps(bitrate));
    if (PHY_HEADER_LENGTH < 0)
        duration = SIMTIME_DBL(modType->getDuration(bits));
    else
        duration = SIMTIME_DBL(modType->getDataMode()->getDuration(bits)) + PHY_HEADER_LENGTH;

    EV_DEBUG << " duration=" << duration * 1e6 << "us(" << bits << "bits " << bitrate / 1e6 << "Mbps)" << endl;
    return duration;
}

void Ieee80211Mac::logState()
{
    Ieee80211Edca::EdcaVector edcCAF = edca->getEdcCaf();
    int numCategs = edca->numCategories();
    EV_TRACE << "# state information: mode = " << modeName(mode) << ", state = " << fsm.getStateName();
    EV_TRACE << ", backoff 0.." << numCategs << " =";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << edcCAF[i].backoff;
    EV_TRACE << "\n# backoffPeriod 0.." << numCategs << " =";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << edcCAF[i].backoffPeriod;
    EV_TRACE << "\n# retryCounter 0.." << numCategs << " =";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << edcCAF[i].retryCounter;
    EV_TRACE << ", radioMode = " << radio->getRadioMode()
             << ", receptionState = " << radio->getReceptionState()
             << ", transmissionState = " << radio->getTransmissionState()
             << ", nav = " << nav << ", txop is " << txop << "\n";
    EV_TRACE << "#queue size 0.." << numCategs << " =";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << edca->transmissionQueue(i)->size();
    EV_TRACE << ", medium is " << (isMediumFree() ? "free" : "busy") << ", scheduled AIFS are";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << i << "(" << (edcCAF[i].endAIFS->isScheduled() ? "scheduled" : "") << ")";
    EV_TRACE << ", scheduled backoff are";
    for (int i = 0; i < numCategs; i++)
        EV_TRACE << " " << i << "(" << (edcCAF[i].endBackoff->isScheduled() ? "scheduled" : "") << ")";
    EV_TRACE << "\n# currentAC: " << edca->getCurrentAc() << ", edca->getOldcurrentAc(): " << edca->getOldCurrentAc();
    if (edca->getCurrentTransmission() != nullptr)
        EV_TRACE << "\n# current transmission: " << edca->getCurrentTransmission()->getId();
    else
        EV_TRACE << "\n# current transmission: none";
    EV_TRACE << endl;
}

const char *Ieee80211Mac::modeName(int mode)
{
#define CASE(x)    case x: \
        s = #x; break
    const char *s = "???";
    switch (mode) {
        CASE(DCF);
        CASE(PCF);
    }
    return s;
#undef CASE
}

void Ieee80211Mac::flushQueue()
{
    if (queueModule) {
        while (!queueModule->isEmpty()) {
            cMessage *msg = queueModule->pop();
            //TODO emit(dropPkIfaceDownSignal, msg); -- 'pkDropped' signals are missing in this module!
            delete msg;
        }
        queueModule->clear();    // clear request count
    }

    for (int i = 0; i < edca->numCategories(); i++) {
        while (!edca->transmissionQueue(i)->empty()) {
            cMessage *msg = edca->transmissionQueue(i)->front();
            edca->transmissionQueue(i)->pop_front();
            //TODO emit(dropPkIfaceDownSignal, msg); -- 'pkDropped' signals are missing in this module!
            delete msg;
        }
    }
}

void Ieee80211Mac::clearQueue()
{
    if (queueModule) {
        queueModule->clear();    // clear request count
    }

    for (int i = 0; i < edca->numCategories(); i++) {
        while (!edca->transmissionQueue(i)->empty()) {
            cMessage *msg = edca->transmissionQueue(i)->front();
            edca->transmissionQueue(i)->pop_front();
            delete msg;
        }
    }
}

bool Ieee80211Mac::needRecoveryFallback(void)
{
    if (edca->retryCounter() == 1) {
        return true;
    }
    else {
        return false;
    }
}

bool Ieee80211Mac::needNormalFallback(void)
{
    int retryMod = (edca->retryCounter() - 1) % 2;
    if (retryMod == 1) {
        return true;
    }
    else {
        return false;
    }
}

double Ieee80211Mac::getBitrate()
{
    return dataFrameMode->getDataMode()->getNetBitrate().get();
}

void Ieee80211Mac::setBitrate(double rate)
{
    dataFrameMode = modeSet->getMode(bps(rate));
}

const IIeee80211Mode *Ieee80211Mac::getControlAnswerMode(const IIeee80211Mode *reqMode)
{
    /**
     * The standard has relatively unambiguous rules for selecting a
     * control response rate (the below is quoted from IEEE 802.11-2007,
     * Section 9.6):
     *
     *   To allow the transmitting STA to calculate the contents of the
     *   Duration/ID field, a STA responding to a received frame shall
     *   transmit its Control Response frame (either CTS or ACK), other
     *   than the BlockAck control frame, at the highest rate in the
     *   BSSBasicRateSet parameter that is less than or equal to the
     *   rate of the immediately previous frame in the frame exchange
     *   sequence (as defined in 9.12) and that is of the same
     *   modulation class (see 9.6.1) as the received frame...
     */

    /**
     * If no suitable basic rate was found, we search the mandatory
     * rates. The standard (IEEE 802.11-2007, Section 9.6) says:
     *
     *   ...If no rate contained in the BSSBasicRateSet parameter meets
     *   these conditions, then the control frame sent in response to a
     *   received frame shall be transmitted at the highest mandatory
     *   rate of the PHY that is less than or equal to the rate of the
     *   received frame, and that is of the same modulation class as the
     *   received frame. In addition, the Control Response frame shall
     *   be sent using the same PHY options as the received frame,
     *   unless they conflict with the requirement to use the
     *   BSSBasicRateSet parameter.
     *
     * TODO: Note that we're ignoring the last sentence for now, because
     * there is not yet any manipulation here of PHY options.
     */
    bool found = false;
    const IIeee80211Mode *bestMode;
    const IIeee80211Mode *mode = modeSet->getSlowestMode();
    while (mode != nullptr) {
        /* If the rate:
         *
         *  - is a mandatory rate for the PHY, and
         *  - is equal to or faster than our current best choice, and
         *  - is less than or equal to the rate of the received frame, and
         *  - is of the same modulation class as the received frame
         *
         * ...then it's our best choice so far.
         */
        if (modeSet->getIsMandatory(mode) &&
            (!found || mode->getDataMode()->getGrossBitrate() > bestMode->getDataMode()->getGrossBitrate()) &&
            mode->getDataMode()->getGrossBitrate() <= reqMode->getDataMode()->getGrossBitrate() &&
            // TODO: same modulation class
            typeid(*mode) == typeid(*bestMode))
        {
            bestMode = mode;
            // As above; we've found a potentially-suitable transmit
            // rate, but we need to continue and consider all the
            // mandatory rates before we can be sure we've got the right
            // one.
            found = true;
        }
    }

    /**
     * If we still haven't found a suitable rate for the response then
     * someone has messed up the simulation config. This probably means
     * that the WifiPhyStandard is not set correctly, or that a rate that
     * is not supported by the PHY has been explicitly requested in a
     * WifiRemoteStationManager (or descendant) configuration.
     *
     * Either way, it is serious - we can either disobey the standard or
     * fail, and I have chosen to do the latter...
     */
    if (!found) {
        throw cRuntimeError("Can't find response rate for reqMode. Check standard and selected rates match.");
    }

    return bestMode;
}

// This methods implemet the duplicate filter
void Ieee80211Mac::sendUp(cMessage *msg)
{
    if (isDuplicated(msg))
        EV_INFO << "Discarding duplicate message\n";
    else {
        EV_INFO << "Sending up " << msg << "\n";
        if (msg->isPacket())
            emit(packetSentToUpperSignal, msg);
        send(msg, upperLayerOutGateId);
    }
}

void Ieee80211Mac::removeOldTuplesFromDuplicateMap()
{
    if (duplicateDetect && lastTimeDelete + duplicateTimeOut >= simTime()) {
        lastTimeDelete = simTime();
        for (auto it = asfTuplesList.begin(); it != asfTuplesList.end(); ) {
            if (it->second.receivedTime + duplicateTimeOut < simTime()) {
                auto itAux = it;
                it++;
                asfTuplesList.erase(itAux);
            }
            else
                it++;
        }
    }
}

const MACAddress& Ieee80211Mac::isInterfaceRegistered()
{
    if (!par("multiMac").boolValue())
        return MACAddress::UNSPECIFIED_ADDRESS;

    IInterfaceTable *ift = findModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    if (!ift)
        return MACAddress::UNSPECIFIED_ADDRESS;
    cModule *interfaceModule = findModuleUnderContainingNode(this);
    if (!interfaceModule)
        throw cRuntimeError("NIC module not found in the host");
    std::string interfaceName = utils::stripnonalnum(interfaceModule->getFullName());
    InterfaceEntry *e = ift->getInterfaceByName(interfaceName.c_str());
    if (e)
        return e->getMacAddress();
    return MACAddress::UNSPECIFIED_ADDRESS;
}

bool Ieee80211Mac::isDuplicated(cMessage *msg)
{
    if (duplicateDetect) {    // duplicate detection filter
        Ieee80211DataOrMgmtFrame *frame = dynamic_cast<Ieee80211DataOrMgmtFrame *>(msg);
        if (frame) {
            auto it = asfTuplesList.find(frame->getTransmitterAddress());
            if (it == asfTuplesList.end()) {
                Ieee80211ASFTuple tuple;
                tuple.receivedTime = simTime();
                tuple.sequenceNumber = frame->getSequenceNumber();
                tuple.fragmentNumber = frame->getFragmentNumber();
                asfTuplesList.insert(std::pair<MACAddress, Ieee80211ASFTuple>(frame->getTransmitterAddress(), tuple));
            }
            else {
                // check if duplicate
                if (it->second.sequenceNumber == frame->getSequenceNumber()
                    && it->second.fragmentNumber == frame->getFragmentNumber())
                {
                    return true;
                }
                else {
                    // actualize
                    it->second.sequenceNumber = frame->getSequenceNumber();
                    it->second.fragmentNumber = frame->getFragmentNumber();
                    it->second.receivedTime = simTime();
                }
            }
        }
    }
    return false;
}

void Ieee80211Mac::promiscousFrame(cMessage *msg)
{
    if (!isDuplicated(msg)) // duplicate detection filter
        emit(NF_LINK_PROMISCUOUS, msg);
}

bool Ieee80211Mac::isBackoffPending()
{
    const Ieee80211Edca::EdcaVector edcCAF = edca->getEdcCaf();
    for (auto & elem : edcCAF) {
        if (elem.backoff)
            return true;
    }
    return false;
}

simtime_t Ieee80211Mac::controlFrameTxTime(unsigned int dataBitLength)
{
    simtime_t duration = controlFrameMode->getDuration(dataBitLength);
    EV_DEBUG << "Control frame duration = " << duration.dbl() << "us (" << dataBitLength << "bits " << controlFrameMode->getDataMode()->getNetBitrate() << ")" << endl;
    return duration;
}

bool Ieee80211Mac::handleNodeStart(IDoneCallback *doneCallback)
{
    if (!doneCallback)
        return true; // do nothing when called from initialize() //FIXME It's a hack, should remove the initializeQueueModule() and setRadioMode() calls from initialize()

    bool ret = MACProtocolBase::handleNodeStart(doneCallback);
    initializeQueueModule();
    radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
    return ret;
}

bool Ieee80211Mac::handleNodeShutdown(IDoneCallback *doneCallback)
{
    bool ret = MACProtocolBase::handleNodeStart(doneCallback);
    handleNodeCrash();
    return ret;
}

void Ieee80211Mac::handleNodeCrash()
{
    cancelEvent(endSIFS);
    cancelEvent(endDIFS);
    cancelEvent(ackTimeout);
    cancelEvent(endReserve);
    cancelEvent(mediumStateChange);
    cancelEvent(endTXOP);
    for (unsigned int i = 0; i < edca->getEdcCaf().size(); i++) {
        cancelEvent(edca->endAIFS(i));
        cancelEvent(edca->endBackoff(i));
        while (!edca->transmissionQueue(i)->empty()) {
            Ieee80211Frame *temp = dynamic_cast<Ieee80211Frame *>(edca->transmissionQueue(i)->front());
            edca->transmissionQueue(i)->pop_front();
            delete temp;
        }
    }
}

void Ieee80211Mac::configureRadioMode(IRadio::RadioMode radioMode)
{
    if (radio->getRadioMode() != radioMode) {
        ConfigureRadioCommand *configureCommand = new ConfigureRadioCommand();
        configureCommand->setRadioMode(radioMode);
        cMessage *message = new cMessage("configureRadioMode", RADIO_C_CONFIGURE);
        message->setControlInfo(configureCommand);
        sendDown(message);
    }
}

} // namespace ieee80211

} // namespace inet

