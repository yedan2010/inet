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
// 

#ifndef __INET_BASICMSDUAGGREGATIONPOLICY_H
#define __INET_BASICMSDUAGGREGATIONPOLICY_H

#include "inet/linklayer/ieee80211/mac/contract/IMsduAggregationPolicy.h"

namespace inet {
namespace ieee80211 {

class INET_API BasicMsduAggregationPolicy : public IMsduAggregationPolicy, public cSimpleModule
{
    protected:
        bool qOsCheck = false;
        int subframeNumThreshold = -1;
        int aggregationLengthThreshold = -1;
        int maxAMsduSize = -1;

    protected:
        virtual void initialize() override;
        virtual bool isAggregationPossible(int numOfFramesToAggragate, int aMsduLength);
        virtual bool isEligible(Ieee80211DataFrame *frame, Ieee80211DataFrame *testFrame, int aMsduLength);

    public:
        virtual std::vector<Ieee80211DataFrame *> *computeAggregateFrames(cQueue *queue) override;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_BASICMSDUAGGREGATIONPOLICY_H
