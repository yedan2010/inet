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

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"

namespace inet {
namespace ieee80211 {

Define_Module(Hcf);

void Hcf::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        auto rateSelection = check_and_cast<IRateSelection *>(getModuleByPath(par("rateSelectionModule")));
        modeSet = rateSelection->getModeSet();
        macDataService = check_and_cast<OriginatorQoSMacDataService *>(getModuleByPath(par("originatorMacDataServiceModule")));
        originatorBlockAckAgreementHandler = check_and_cast<OriginatorBlockAckAgreementHandler *>(getModuleByPath(par("originatorBlockAckAgreementHandlerModule")));
        lifetimeHandler = new EdcaTransmitLifetimeHandler(0, 0, 0, 0); // FIXME: needs only one timeout parameter
        ackProcedure = new OriginatorAckProcedure(rateSelection);
        rtsProcedure = new RtsProcedure(rateSelection);
        blockAckProcedure = new OriginatorBlockAckProcedure(rateSelection);
        simtime_t txopLimit = par("txopLimit");
        txopProcedure = new TxOpProcedure(TxOpProcedure::Type::EDCA, txopLimit != -1 ? txopLimit : getTxopLimit(rateSelection->getSlowestMandatoryMode()).get());
        for (int i = 0; i < 4; i++) {
            pendingQueues.push_back(new PendingQueue(par("maxQueueSize"), nullptr));
            ackHandlers.push_back(new AckHandler());
            inprogressFrames.push_back(new InProgressFrames(pendingQueues[i], macDataService, ackHandlers[i]));
            recoveryProcedures.push_back(getSubmodule("recoveryProcedure", i));
        }
    }
}

void Hcf::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    edca->processUpperFrame(frame);
}

void Hcf::processLowerFrame(Ieee80211Frame* frame)
{
    edca->processLowerFrame(frame);
}

void Hcf::channelAccessGranted()
{
    auto edcafs = edca->getEdcafs();
    for (auto edcaf : edcafs) {
        if (edcaf->isOwning()) {
            AccessCategory ac = edcaf->getAccessCategory();
            // start frame sequence
        }
        else if (edcaf->isInternalCollision()) {
            Ieee80211DataOrMgmtFrame *internallyCollidedFrame = inProgressFrames->getFrameToTransmit();
            recoveryProcedure->dataOrMgmtFrameTransmissionFailed(internallyCollidedFrame);
            if (recoveryProcedure->isDataOrMgtmFrameRetryLimitReached(internallyCollidedFrame))
                inProgressFrames->dropFrame(internallyCollidedFrame);
            else
                edcaf->requestChannelAccess(this, 2234234);
        }
        else ;
    }
}

void Hcf::transmitFrame(Ieee80211Frame* frame, simtime_t ifs)
{
}

void Hcf::frameSequenceFinished()
{
}

bool Hcf::isReceptionInProgress()
{
}

bool Hcf::transmissionComplete()
{
}

} // namespace ieee80211
} // namespace inet

