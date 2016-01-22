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

#include "BasicMsduAggregationPolicy.h"

namespace inet {
namespace ieee80211 {

Define_Module(BasicMsduAggregationPolicy);

void BasicMsduAggregationPolicy::initialize()
{
    subframeNumThreshold = par("subframeNumThreshold");
    aggregationLengthThreshold = par("aggregationLengthThreshold");
    maxAMsduSize = par("maxAMsduSize");
    qOsCheck = par("qOsCheck");
}

bool BasicMsduAggregationPolicy::isAggregationPossible(int numOfFramesToAggragate, int aMsduLength)
{
    return ((subframeNumThreshold == -1 || subframeNumThreshold <= numOfFramesToAggragate) &&
            (aggregationLengthThreshold == -1 || aggregationLengthThreshold <= aMsduLength));
}


bool BasicMsduAggregationPolicy::isEligible(Ieee80211DataFrame* frame, Ieee80211DataFrame* testFrame, int aMsduLength)
{
//   Only QoS data frames have a TID.
    if (qOsCheck && frame->getType() != ST_DATA_WITH_QOS)
        return false;

//    The maximum MPDU length that can be transported using A-MPDU aggregation is 4095 octets. An
//    A-MSDU cannot be fragmented. Therefore, an A-MSDU of a length that exceeds 4065 octets (
//    4095 minus the QoS data MPDU overhead) cannot be transported in an A-MPDU.
    if (aMsduLength + testFrame->getEncapsulatedPacket()->getByteLength() + LENGTH_A_MSDU_SUBFRAME_HEADER / 8 > maxAMsduSize) // default value of maxAMsduSize is 4065
        return false;

//    The value of TID present in the QoS Control field of the MPDU carrying the A-MSDU indicates the TID for
//    all MSDUs in the A-MSDU. Because this value of TID is common to all MSDUs in the A-MSDU, only MSDUs
//    delivered to the MAC by an MA-UNITDATA.request primitive with an integer priority parameter that maps
//    to the same TID can be aggregated together using A-MSDU.
    if (testFrame->getTid() != frame->getTid())
        return false;

//    An A-MSDU contains only MSDUs whose DA and SA parameter values map to the same receiver address
//    (RA) and transmitter address (TA) values, i.e., all the MSDUs are intended to be received by a single
//    receiver, and necessarily they are all transmitted by the same transmitter. The rules for determining RA and
//    TA are independent of whether the frame body carries an A-MSDU.
    if (testFrame->getReceiverAddress() != frame->getReceiverAddress() ||
        testFrame->getTransmitterAddress() != frame->getTransmitterAddress())
        return false;

    return true;
}

} /* namespace ieee80211 */
} /* namespace inet */

