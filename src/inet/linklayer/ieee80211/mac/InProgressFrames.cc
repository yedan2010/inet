//
// Copyright (C) 2015 Andras Varga
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
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#include "InProgressFrames.h"

namespace inet {
namespace ieee80211 {

void InProgressFrames::ensureFilled()
{
//    TODO: delete old frames from inProgressFrames
//    if (auto dataFrame = dynamic_cast<Ieee80211DataFrame*>(frame)) {
//        if (transmitLifetimeHandler->isLifetimeExpired(dataFrame))
//            return frame;
//    }
    if (inProgressFrames.empty()) { // TODO: this condition is not appropriate <-> hasEligibleFrames() or something
        auto frames = extractor->extractFramesToTransmit(pendingQueue);
        for (auto frame : frames)
            inProgressFrames.push_back(frame);
    }
}
bool InProgressFrames::isEligibleStatusToTransmit(AckHandler::Status status)
{
    return status == AckHandler::Status::ARRIVED_UNACKED || status == AckHandler::Status::NOT_ARRIVED_IN_TIME || status == AckHandler::Status::UNKNOWN;
}

Ieee80211DataOrMgmtFrame *InProgressFrames::getFrameToTransmit()
{
    ensureFilled();
    for (auto frame : inProgressFrames) {
        AckHandler::Status ackStatus = ackHandler->getAckStatus(frame);
        if (isEligibleStatusToTransmit(ackStatus))
            return frame;
    }
    return nullptr;
}

void InProgressFrames::dropAndDeleteFrame(int seqNum, int fragNum)
{
    for (auto it = inProgressFrames.begin(); it != inProgressFrames.end(); it++) {
        Ieee80211DataOrMgmtFrame *frame = *it;
        if (frame->getSequenceNumber() == seqNum && frame->getFragmentNumber() == fragNum) {
            inProgressFrames.erase(it);
            delete frame;
            break;
        }
    }
}

void InProgressFrames::dropAndDeleteFrame(Ieee80211DataOrMgmtFrame* frame)
{
    inProgressFrames.remove(frame);
    delete frame;
}

void InProgressFrames::dropAndDeleteFrames(std::set<std::pair<uint16_t, uint8_t>> seqAndFragNums)
{
    SequenceControlPredicate predicate(seqAndFragNums);
    inProgressFrames.remove_if(predicate);
    const std::vector<const Ieee80211DataOrMgmtFrame *>& framesToDelete = predicate.getFramesToDelete();
    for (auto frame : framesToDelete)
        delete frame;
}

} /* namespace ieee80211 */
} /* namespace inet */
