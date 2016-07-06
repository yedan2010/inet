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

#include "inet/linklayer/ieee80211/mac/coordinationfunction/Hcf.h"

namespace inet {
namespace ieee80211 {

Hcf::Hcf(Edca *edca, Hcca *hcca)
{
    this->edca = edca;
    this->hcca = hcca;
}

void Hcf::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    edca->processUpperFrame(frame);
}

void Hcf::processLowerFrame(Ieee80211Frame* frame)
{
    edca->processUpperFrame(frame);
}

void Hcf::channelAccessGranted(IContentionBasedChannelAccess* channelAccess)
{
    edca->channelAccessGranted(channelAccess);
}

void Hcf::internalCollision(IContentionBasedChannelAccess* channelAccess)
{
    edca->internalCollision(channelAccess);
}

void Hcf::channelAccessGranted(IContentionFreeChannelAccess* channelAccess)
{
    throw cRuntimeError("Hcca is unimplemented!");
}

} // namespace ieee80211
} // namespace inet

