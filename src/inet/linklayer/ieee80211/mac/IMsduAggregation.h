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

#ifndef __INET_IMSDUAGGREGATION_H
#define __INET_IMSDUAGGREGATION_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

class INET_API IMsduAggregation
{
    public:
        virtual Ieee80211Frame *createAggregateFrame(cQueue *queue) = 0;
        virtual std::vector<Ieee80211DataFrame *> explodeAggregateFrame(Ieee80211DataFrame *frame) = 0;
        virtual ~IMsduAggregation() {}
};

} // namespace ieee80211
} // namespace inet

#endif
