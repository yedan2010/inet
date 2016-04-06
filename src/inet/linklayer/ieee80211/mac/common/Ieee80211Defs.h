//
// Copyright (C) 2015 OpenSim Ltd.
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
// Author: Andras Varga, Benjamin Seregi
//

#ifndef __INET_IEEE80211DEFS_H
#define __INET_IEEE80211DEFS_H

#include "inet/common/INETDefs.h"

namespace inet {
namespace ieee80211 {

typedef int16_t SequenceNumber;
typedef int8_t FragmentNumber;
typedef int8_t Tid;

} // namespace ieee80211
} // namespace inet

#endif

