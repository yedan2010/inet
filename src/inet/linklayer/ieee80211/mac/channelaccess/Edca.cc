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

#include "Edca.h"

namespace inet {
namespace ieee80211 {

Define_Module(Edca);

void Edca::initialize(int stage)
{
    if (stage == INITSTAGE_LINK_LAYER_2) {
        int numEdcafs = par("numEdcafs");
        for (int ac = 0; ac < numEdcafs; ac++) {
            auto callback = check_and_cast<IChannelAccess::ICallback*>(getParentModule());
            // edcafs.push_back(new Edcaf(-1, ac, callback, collisionController, nullptr));
        }
    }
}

Tid Edca::getTid(Ieee80211DataOrMgmtFrame* frame)
{
    if (auto addbaResponse = dynamic_cast<Ieee80211AddbaResponse*>(frame)) {
        return addbaResponse->getTid();
    }
    else if (auto qosDataFrame = dynamic_cast<Ieee80211DataFrame*>(frame)) {
        ASSERT(qosDataFrame->getType() == ST_DATA_WITH_QOS);
        return qosDataFrame->getTid();
    }
    else
        throw cRuntimeError("Unknown frame type = %d", frame->getType());
}

AccessCategory Edca::classifyFrame(Ieee80211DataOrMgmtFrame *frame)
{
    return mapTidToAc(getTid(frame));
}

// TODO: duplicate code
AccessCategory Edca::mapTidToAc(Tid tid)
{
    // standard static mapping (see "UP-to-AC mappings" table in the 802.11 spec.)
    switch (tid) {
        case 1: case 2: return AC_BK;
        case 0: case 3: return AC_BE;
        case 4: case 5: return AC_VI;
        case 6: case 7: return AC_VO;
        default: throw cRuntimeError("No mapping from TID=%d to AccessCategory (must be in the range 0..7)", tid);
    }
}

Edcaf* Edca::getChannelOwner()
{
    for (auto edcaf : edcafs)
        if (edcaf->isOwning())
            return edcaf;
    return nullptr;
}

std::vector<Edcaf*> Edca::getInternallyCollidedEdcafs()
{
    std::vector<Edcaf*> edcafs;
    for (auto edcaf : this->edcafs)
        if (edcaf->isInternalCollision())
            edcafs.push_back(edcaf);
    return edcafs;
}

void Edca::requestChannelAccess(AccessCategory ac, IChannelAccess::ICallback* callback)
{
    edcafs[ac]->requestChannelAccess(callback);
}

void Edca::releaseChannelAccess(AccessCategory ac, IChannelAccess::ICallback* callback)
{
    edcafs[ac]->releaseChannelAccess(callback);
}

} // namespace ieee80211
} // namespace inet
