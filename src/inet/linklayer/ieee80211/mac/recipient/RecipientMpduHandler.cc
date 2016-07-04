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
#include "inet/linklayer/ieee80211/mac/Ieee80211Mac.h"
#include "RecipientMpduHandler.h"

namespace inet {
namespace ieee80211 {

Define_Module(RecipientMpduHandler);

void RecipientMpduHandler::initialize(int stage)
{
    if (stage == INITSTAGE_LAST) {
        mac = check_and_cast<Ieee80211Mac *>(getContainingNicModule(this));
        macDataService = check_and_cast<RecipientMacDataService*>(getModuleByPath(par("recipientDataServiceModule")));
        auto rx = check_and_cast<IRx *>(getModuleByPath(par("rxModule")));
        tx = check_and_cast<ITx *>(getModuleByPath(par("txModule")));
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        ackProcedure = new AckProcedure(rateSelection);
        ctsProcedure = new CtsProcedure(rx, rateSelection);

        sifs = rateSelection->getSlowestMandatoryMode()->getSifsTime();
    }
}

void RecipientMpduHandler::processControlFrame(Ieee80211Frame* frame) // TODO: control frame type
{
    if (auto rtsFrame = dynamic_cast<Ieee80211RTSFrame *>(frame)) {
        ctsProcedure->processReceivedRts(rtsFrame);
        auto ctsFrame = ctsProcedure->buildCts(rtsFrame);
        if (ctsFrame) {
            tx->transmitFrame(ctsFrame, sifs, this);
            ctsProcedure->processTransmittedCts(ctsFrame);
        }
    }
}

void RecipientMpduHandler::processReceivedFrame(Ieee80211Frame* frame)
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
    else { // TODO: else if (auto ctrlFrame = dynamic_cast<Ieee80211ControlFrame*>(frame))
        sendUp(macDataService->controlFrameReceived(frame));
        processControlFrame(frame);
    }
    // else
    //  throw cRuntimeError("Unknown 802.11 Frame");
}


void RecipientMpduHandler::sendUp(const std::vector<Ieee80211Frame*>& completeFrames)
{
    for (auto frame : completeFrames)
        mac->sendUp(frame);
}

// FIXME: revise this
void RecipientMpduHandler::transmissionComplete()
{
    // void
}

} /* namespace ieee80211 */
} /* namespace inet */
