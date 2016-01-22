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

#include "TxRetryHandler.h"

namespace inet {
namespace ieee80211 {

//
// Contention window management
// ============================
//
// The CW shall take the next value in the series every time an
// unsuccessful attempt to transmit an MPDU causes either STA retry
// counter to increment, until the CW reaches the value of aCWmax.
//
// The CW shall be reset to aCWmin after [...] when SLRC reaches
// dot11LongRetryLimit, or when SSRC reaches dot11ShortRetryLimit.
//
void TxRetryHandler::incrementStationSrc()
{
    stationShortRetryCounter++;
    if (stationShortRetryCounter == params->getShortRetryLimit()) // 9.3.3 Random backoff time
        resetContentionWindow();
    else
        cw = doubleCw(cw);
}

void TxRetryHandler::incrementStationLrc()
{
    stationLongRetryCounter++;
    if (stationLongRetryCounter == params->getLongRetryLimit()) // 9.3.3 Random backoff time
        resetContentionWindow();
    else
        cw = doubleCw(cw);
}

void TxRetryHandler::incrementCounter(Ieee80211Frame* frame, std::map<long int, int>& retryCounter)
{
    int id = frame->getTreeId();
    if (retryCounter.find(id) != retryCounter.end())
        retryCounter[id]++;
    else
        retryCounter.insert(std::make_pair(id, 1));
}

int TxRetryHandler::doubleCw(int cw)
{
    int newCw = 2 * cw + 1;
    if (newCw > timingParameters->getCwMax())
        return timingParameters->getCwMax();
    return newCw;
}

//
// The SSRC shall be reset to 0 [...] when a frame with a group address in the
// Address1 field is transmitted. The SLRC shall be reset to 0 when [...] a
// frame with a group address in the Address1 field is transmitted.
//
void TxRetryHandler::multicastFrameTransmitted()
{
    resetStationLrc();
    resetStationSrc();
}

//
// The SSRC shall be reset to 0 when a CTS frame is received in response to an RTS
// frame, when a BlockAck frame is received in response to a BlockAckReq frame, when
// an ACK frame is received in response to the transmission of a frame of length greater*
// than dot11RTSThreshold containing all or part of an MSDU or MMPDU, [...]
// The SLRC shall be reset to 0 when an ACK frame is received in response to transmission
// of a frame containing all or part of an MSDU or MMPDU of [...]
//
// Note: * This is obviously wrong.
//
void TxRetryHandler::ctsFrameReceived()
{
    resetStationSrc();
}

void TxRetryHandler::blockAckFrameReceived()
{
    resetStationSrc();
}

//
// This SRC and the SSRC shall be reset when a MAC frame of length less than or equal
// to dot11RTSThreshold succeeds for that MPDU of type Data or MMPDU.

// This LRC and the SLRC shall be reset when a MAC frame of length greater than dot11RTSThreshold
// succeeds for that MPDU of type Data or MMPDU.
//
void TxRetryHandler::ackFrameReceived(Ieee80211DataOrMgmtFrame *ackedFrame)
{
    if (ackedFrame->getByteLength() >= params->getRtsThreshold())
        resetStationLrc();
    else
        resetStationSrc();
//
// The CW shall be reset to aCWmin after every successful attempt to transmit a frame containing
// all or part of an MSDU or MMPDU
//
    resetContentionWindow();
}

//
// The SRC for an MPDU of type Data or MMPDU and the SSRC shall be incremented every
// time transmission of a MAC frame of length less than or equal to dot11RTSThreshold
// fails for that MPDU of type Data or MMPDU.

// The LRC for an MPDU of type Data or MMPDU and the SLRC shall be incremented every time
// transmission of a MAC frame of length greater than dot11RTSThreshold fails for that MPDU
// of type Data or MMPDU.
//
void TxRetryHandler::dataOrMgmtFrameTransmissionFailed(Ieee80211DataOrMgmtFrame *failedFrame)
{
    if (failedFrame->getByteLength() >= params->getRtsThreshold()) {
        incrementStationLrc();
        incrementCounter(failedFrame, longRetryCounter);
    }
    else {
        incrementStationSrc();
        incrementCounter(failedFrame, shortRetryCounter);
    }
}

//
// If the RTS transmission fails, the SRC for the MSDU or MMPDU and the SSRC are incremented.
//
void TxRetryHandler::rtsFrameTransmissionFailed(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    incrementStationSrc();
    incrementCounter(protectedFrame, shortRetryCounter);
}

//
// Retries for failed transmission attempts shall continue until the SRC for the MPDU of type
// Data or MMPDU is equal to dot11ShortRetryLimit or until the LRC for the MPDU of type Data
// or MMPDU is equal to dot11LongRetryLimit. When either of these limits is reached, retry attempts
// shall cease, and the MPDU of type Data (and any MSDU of which it is a part) or MMPDU shall be discarded.
//
bool TxRetryHandler::isDataOrMgtmFrameRetryLimitReached(Ieee80211DataOrMgmtFrame* failedFrame)
{
    if (failedFrame->getByteLength() >= params->getRtsThreshold())
        return getRc(failedFrame, longRetryCounter) >= params->getLongRetryLimit();
    else
        return getRc(failedFrame, shortRetryCounter) >= params->getShortRetryLimit();
}

bool TxRetryHandler::isRtsFrameRetryLimitReached(Ieee80211DataOrMgmtFrame* protectedFrame)
{
    return getRc(protectedFrame, shortRetryCounter) >= params->getShortRetryLimit();
}

bool TxRetryHandler::isLifetimeExpired(Ieee80211DataOrMgmtFrame* dataOrMgmtFrame)
{
    throw cRuntimeError("Unimplemented");
}


int TxRetryHandler::getRc(Ieee80211Frame* frame, std::map<long int, int>& retryCounter)
{
    auto count = retryCounter.find(frame->getTreeId());
    if (count != retryCounter.end())
        return count->second;
    else
        throw cRuntimeError("The retry counter entry doesn't exist for message id: %d", frame->getId());
}

bool TxRetryHandler::isMulticastFrame(Ieee80211Frame* frame)
{
    if (dynamic_cast<Ieee80211OneAddressFrame*>(frame)) {
        Ieee80211OneAddressFrame *oneAddressFrame = dynamic_cast<Ieee80211OneAddressFrame*>(frame);
        return oneAddressFrame->getReceiverAddress().isBroadcast();
    }
    return false;
}

} /* namespace ieee80211 */
} /* namespace inet */
