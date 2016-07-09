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

#include "inet/common/ModuleAccess.h"
#include "inet/linklayer/ieee80211/mac/contract/IRateSelection.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "RecipientQoSMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(RecipientQoSMpduHandler);

void RecipientQoSMpduHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        mac = check_and_cast<Ieee80211Mac *>(getContainingNicModule(this));
        macDataService = check_and_cast<RecipientQoSMacDataService*>(getModuleByPath(par("recipientMacDataServiceModule")));
        recipientBlockAckAgreementHandler = check_and_cast<RecipientBlockAckAgreementHandler*>(getModuleByPath(par("recipientBlockAckAgreementHandler")));
        originatorBlockAckAgreementHandler = check_and_cast<OriginatorBlockAckAgreementHandler*>(getModuleByPath(par("originatorBlockAckAgreementHandlerModule")));
        auto rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        ackProcedure = new AckProcedure(rateSelection);
        ctsProcedure = new CtsProcedure(rx, rateSelection);
        blockAckProcedure = new BlockAckProcedure(recipientBlockAckAgreementHandler, rateSelection);
        sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
    }
}

void RecipientQoSMpduHandler::processManagementFrame(Ieee80211ManagementFrame* frame)
{
    if (auto addbaRequest = dynamic_cast<Ieee80211AddbaRequest *>(frame)) {
        recipientBlockAckAgreementHandler->processReceivedAddbaRequest(addbaRequest);
        auto addbaResponse = recipientBlockAckAgreementHandler->buildAddbaResponse(addbaRequest);
        if (addbaResponse) {
            auto hcf = check_and_cast<Hcf*>(getParentModule()); // FIXME: khm
            hcf->processUpperFrame(addbaResponse);
        }
    }
    else if (auto addbaResponse = dynamic_cast<Ieee80211AddbaResponse *>(frame)) {
        originatorBlockAckAgreementHandler->processReceivedAddbaResponse(addbaResponse);
    }
    else if (auto delba = dynamic_cast<Ieee80211Delba*>(frame)) {
        if (delba->getInitiator())
            recipientBlockAckAgreementHandler->processReceivedDelba(delba);
        else
            originatorBlockAckAgreementHandler->processReceivedDelba(delba);
    }
    else
        throw cRuntimeError("Unknown packet");
}

void RecipientQoSMpduHandler::processControlFrame(Ieee80211Frame* frame) // TODO: control frame type
{
    if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
        ctsProcedure->processReceivedRts(rtsFrame);
        auto ctsFrame = ctsProcedure->buildCts(rtsFrame);
        if (ctsFrame) {
            tx->transmitFrame(ctsFrame, sifs, this);
            ctsProcedure->processTransmittedCts(ctsFrame);
        }
    }
    else if (auto blockAckRequest = dynamic_cast<Ieee80211BlockAckReq*>(frame)) {
        blockAckProcedure->processReceivedBlockAckReq(blockAckRequest);
        auto blockAck = blockAckProcedure->buildBlockAck(blockAckRequest);
        if (blockAck) {
            tx->transmitFrame(blockAck, sifs, this);
            blockAckProcedure->processTransmittedBlockAck(blockAck);
        }
    }
    else
        throw cRuntimeError("Unknown packet");
}

void RecipientQoSMpduHandler::processReceivedFrame(Ieee80211Frame* frame)
{
    ackProcedure->processReceivedFrame(frame);
    auto ack = ackProcedure->buildAck(frame);
    if (ack) {
        tx->transmitFrame(ack, sifs, this);
        ackProcedure->processTransmittedAck(ack);
    }
    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame*>(frame)) {
        sendUp(macDataService->dataFrameReceived(dataFrame));
    }
    else if (auto mgmtFrame = dynamic_cast<Ieee80211ManagementFrame*>(frame)) {
        sendUp(macDataService->managementFrameReceived(mgmtFrame));
        processManagementFrame(mgmtFrame);
    }
    else { // TODO: else if (auto ctrlFrame = dynamic_cast<Ieee80211ControlFrame*>(frame))
        sendUp(macDataService->controlFrameReceived(frame));
        processControlFrame(frame);
    }
    // else
    //  throw cRuntimeError("Unknown 802.11 Frame");
}


void RecipientQoSMpduHandler::sendUp(const std::vector<Ieee80211Frame*>& completeFrames)
{
    for (auto frame : completeFrames) {
        // FIXME: mgmt module does not handle addba req ..
        if (!dynamic_cast<Ieee80211AddbaRequest*>(frame) && !dynamic_cast<Ieee80211AddbaResponse*>(frame))
            mac->sendUp(frame);
    }
}

void RecipientQoSMpduHandler::transmissionComplete()
{
    // void
}

} /* namespace ieee80211 */
} /* namespace inet */
