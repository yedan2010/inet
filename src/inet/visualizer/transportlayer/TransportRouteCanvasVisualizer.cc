//
// Copyright (C) 2016 OpenSim Ltd.
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/linklayer/ethernet/switch/MACRelayUnit.h"
#include "inet/linklayer/ieee8021d/relay/Ieee8021dRelay.h"
#include "inet/networklayer/ipv4/IPv4.h"
#include "inet/visualizer/transportlayer/TransportRouteCanvasVisualizer.h"

namespace inet {

namespace visualizer {

Define_Module(TransportRouteCanvasVisualizer);

bool TransportRouteCanvasVisualizer::isPathEnd(cModule *module) const
{
    return dynamic_cast<IPv4 *>(module) != nullptr;
}

bool TransportRouteCanvasVisualizer::isPathElement(cModule *module) const
{
    return dynamic_cast<MACRelayUnit *>(module) != nullptr || dynamic_cast<Ieee8021dRelay *>(module) != nullptr;
}

} // namespace visualizer

} // namespace inet
