//
// Copyright (C) 2015 Opensim Ltd.
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
// Author: Zoltan Bojthe
//

#include "inet/linklayer/ieee80211/mac/fragmentation/Defragmentation.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"

namespace inet {
namespace ieee80211 {

Register_Class(Defragmentation);

Ieee80211DataOrMgmtFrame *Defragmentation::defragmentFrames(std::vector<Ieee80211DataOrMgmtFrame *> *fragmentFrames)
{
    return check_and_cast<Ieee80211DataOrMgmtFrame *>(fragmentFrames->at(fragmentFrames->size() - 1)->decapsulate());
}

} // namespace ieee80211
} // namespace inet

