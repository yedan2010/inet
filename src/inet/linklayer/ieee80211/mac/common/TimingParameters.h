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

#ifndef __INET_TIMINGPARAMETERS_H
#define __INET_TIMINGPARAMETERS_H

#include "inet/common/INETDefs.h"

namespace inet {
namespace ieee80211 {

class TimingParam
{
protected:
    simtime_t slotTime;
    simtime_t sifsTime;
};

class DcfTimingParam
{
protected:
    simtime_t difsTime;
};

class EdcafTimingParam
{
protected:
    simtime_t aifsTime;
    simtime_t eifsTime;
    simtime_t pifsTime;
    simtime_t rifsTime;
    simtime_t txopLimit;
};

class INET_API TimingParameters
{
    protected:
        simtime_t slotTime;
        simtime_t sifsTime;
        simtime_t aifsTime;
        simtime_t eifsTime;
        simtime_t pifsTime;
        simtime_t rifsTime;
        simtime_t txopLimit;

    public:
        TimingParameters(simtime_t slotTime, simtime_t sifsTime, simtime_t aifsTime, simtime_t eifsTime, simtime_t pifsTime, simtime_t rifsTime, simtime_t txopLimit) :
            slotTime(slotTime),
            sifsTime(sifsTime),
            aifsTime(aifsTime),
            eifsTime(eifsTime),
            pifsTime(pifsTime),
            rifsTime(rifsTime),
            txopLimit(txopLimit)
        { }
        virtual ~TimingParameters() {}

        // timing parameters
        virtual simtime_t getSlotTime() const { return slotTime; }
        virtual simtime_t getSifsTime() const { return sifsTime; }
        virtual simtime_t getAifsTime() const { return aifsTime; }
        virtual simtime_t getEifsTime() const { return eifsTime; }
        virtual simtime_t getPifsTime() const { return pifsTime; }
        virtual simtime_t getRifsTime() const { return rifsTime; }
        virtual simtime_t getTxopLimit() const { return txopLimit; }
};

} // namespace ieee80211
} // namespace inet

#endif

