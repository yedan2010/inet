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

#include "MsduAggregation.h"

namespace inet {
namespace ieee80211 {

Define_Module(MsduAggregation);

void MsduAggregation::initialize()
{
    aggregationPolicy = check_and_cast<IMsduAggregationPolicy*>(getSubmodule("aggregationPolicy"));
}

Ieee80211Frame* MsduAggregation::createAggregateFrame(cQueue* queue)
{
    ASSERT(queue->empty() == false);
    int numOfFramesToAggragate = 0;
    int aMsduLength = 0;
    Ieee80211DataFrame *firstFrame = (Ieee80211DataFrame *)queue->front();
    for (cQueue::Iterator it(*queue); !it.end(); it++)
    {
        Ieee80211DataFrame *dataFrame = dynamic_cast<Ieee80211DataFrame *>(it());
        if (!dataFrame || !aggregationPolicy->isEligible(dataFrame, firstFrame, aMsduLength))
            return aggregateIfPossible(queue, numOfFramesToAggragate, aMsduLength);
        numOfFramesToAggragate++;
        aMsduLength += dataFrame->getEncapsulatedPacket()->getByteLength() + LENGTH_A_MSDU_SUBFRAME_HEADER / 8; // sum of MSDU lengths + subframe header
    }
    return aggregateIfPossible(queue, numOfFramesToAggragate, aMsduLength);
}

Ieee80211Frame* MsduAggregation::aggregateIfPossible(cQueue* queue, int numOfFramesToAggragate, int aMsduLength)
{
    if (numOfFramesToAggragate <= 1 || !aggregationPolicy->isAggregationPossible(numOfFramesToAggragate, aMsduLength))
        return (Ieee80211Frame*) queue->pop();
    Ieee80211DataFrame *dataFrame = (Ieee80211DataFrame*) (queue->front());
    int tid = dataFrame->getTid();
    bool toDS = dataFrame->getToDS();
    bool fromDS = dataFrame->getFromDS();
    MACAddress ra = dataFrame->getReceiverAddress();
    MACAddress ta = dataFrame->getTransmitterAddress();
    Ieee80211AMsdu *aMsdu = new Ieee80211AMsdu();
    aMsdu->setSubframesArraySize(numOfFramesToAggragate);
    for (int i = 0; i < numOfFramesToAggragate; i++)
    {
        dataFrame = (Ieee80211DataFrame*) (queue->pop());
        cPacket *msdu = dataFrame->decapsulate();
        Ieee80211MsduSubframe msduSubframe;
        setSubframeAddress(&msduSubframe, dataFrame);
        msduSubframe.encapsulate(msdu);
        aMsdu->setSubframes(i, msduSubframe);
        delete dataFrame;
    }
    aMsdu->setByteLength(aMsduLength);
//    The MPDU containing the A-MSDU is carried in any of the following data frame subtypes: QoS Data,
//    QoS Data + CF-Ack, QoS Data + CF-Poll, QoS Data + CF-Ack + CF-Poll. The A-MSDU structure is
//    contained in the frame body of a single MPDU.
    Ieee80211DataFrame *aggregatedDataFrame = new Ieee80211DataFrame();
//    aggregatedDataFrame->setType(ST_DATA_WITH_QOS); FIXME
    aggregatedDataFrame->setToDS(toDS);
    aggregatedDataFrame->setFromDS(fromDS);
    aggregatedDataFrame->setType(ST_DATA);
    aggregatedDataFrame->setAMsduPresent(true);
    aggregatedDataFrame->setTransmitterAddress(ta);
    aggregatedDataFrame->setReceiverAddress(ra);
    aggregatedDataFrame->setTid(tid);
    aggregatedDataFrame->encapsulate(aMsdu);
    // TODO: set addr3 and addr4 according to fromDS and toDS.
    return aggregatedDataFrame;
}

void MsduAggregation::setSubframeAddress(Ieee80211MsduSubframe *subframe, Ieee80211DataFrame* frame)
{
    // Note: Addr1 (RA), Addr2 (TA)
    // Table 8-19—Address field contents
    MACAddress da, sa;
    bool toDS = frame->getToDS();
    bool fromDS = frame->getFromDS();
    if (toDS == 0 && fromDS == 0) // STA to STA
    {
        da = frame->getReceiverAddress();
        sa = frame->getTransmitterAddress();
    }
    else if (toDS == 0 && fromDS == 1) // AP to STA
    {
        da = frame->getReceiverAddress();
        sa = frame->getAddress3();
    }
    else if (toDS == 1 && fromDS == 0) // STA to AP
    {
        da = frame->getAddress3();
        sa = frame->getTransmitterAddress();
    }
    else if (toDS == 1 && fromDS == 1) // AP to AP
    {
        da = frame->getAddress3();
        sa = frame->getAddress4();
    }
    subframe->setDa(da);
    subframe->setSa(sa);
}

void MsduAggregation::setExplodedFrameAddress(Ieee80211DataFrame* frame, Ieee80211MsduSubframe* subframe, Ieee80211DataFrame *aMsduFrame)
{
    bool toDS = aMsduFrame->getToDS();
    bool fromDS = aMsduFrame->getFromDS();
    if (fromDS == 0 && toDS == 0) // STA to STA
    {
        frame->setTransmitterAddress(aMsduFrame->getTransmitterAddress());
        frame->setReceiverAddress(aMsduFrame->getReceiverAddress());
    }
    else if (fromDS == 1 && toDS == 0) // AP to STA
    {
        frame->setTransmitterAddress(frame->getTransmitterAddress());
        frame->setReceiverAddress(subframe->getDa());
        frame->setAddress3(subframe->getSa());
    }
    else if (fromDS == 0 && toDS == 1) // STA to AP
    {
        frame->setTransmitterAddress(subframe->getSa());
        frame->setReceiverAddress(aMsduFrame->getReceiverAddress());
        frame->setAddress3(subframe->getDa());
    }
    else if (fromDS == 1 && toDS == 1) // AP to AP
    {
        frame->setReceiverAddress(aMsduFrame->getReceiverAddress());
        frame->setTransmitterAddress(aMsduFrame->getTransmitterAddress());
        frame->setAddress3(subframe->getDa());
        frame->setAddress4(subframe->getSa());
    }
}

std::vector<Ieee80211DataFrame*> MsduAggregation::explodeAggregateFrame(Ieee80211DataFrame* frame)
{
    std::vector<Ieee80211DataFrame *> frames;
    Ieee80211AMsdu *aMsdu = dynamic_cast<Ieee80211AMsdu *>(frame->getEncapsulatedPacket());
    int tid = frame->getTid();
    if (aMsdu)
    {
        int numOfSubframes = aMsdu->getSubframesArraySize();
        for (int i = 0; i < numOfSubframes; i++)
        {
            Ieee80211MsduSubframe msduSubframe = aMsdu->getSubframes(i);
            cPacket *msdu = msduSubframe.decapsulate();
            Ieee80211DataFrame *dataFrame = new Ieee80211DataFrame();
//            dataFrame->setType(ST_DATA_WITH_QOS); FIXME:
            dataFrame->setType(ST_DATA);
            dataFrame->setTransmitterAddress(msduSubframe.getSa());
            dataFrame->setToDS(frame->getToDS());
            dataFrame->setFromDS(frame->getFromDS());
            dataFrame->setTid(tid);
            dataFrame->encapsulate(msdu);
            setExplodedFrameAddress(dataFrame, &msduSubframe, frame);
            frames.push_back(dataFrame);
        }
        delete frame;
    }
    else
        frames.push_back(frame);
    return frames;
}

} /* namespace ieee80211 */
} /* namespace inet */

