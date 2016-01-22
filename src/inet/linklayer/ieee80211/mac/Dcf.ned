//
// Copyright (C) 2015 Andras Varga
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
// along with this program; if not, see http://www.gnu.org/licenses/.
//
// Author: Andras Varga
//

package inet.linklayer.ieee80211.mac;

//
// Dcf
//
module Dcf
{
    parameters:
        string macModule;
        string rxModule;
        string txModule;
        string rateSelectionModule;
        string duplicateDetectorModule;
        string collisionControllerModule;

        double slotTime @unit("s") = default(-1s); // slot time, -1 means default (mode dependent)
        double sifsTime @unit("s") = default(-1s); // SIFS time, -1 means default (mode dependent)
        double difsTime @unit("s") = default(-1s); // DIFS time, -1 means default (mode dependent)
        int cwMin = default(-1); // contention window for normal data frames, -1 means default
        int cwMax = default(-1); // contention window for normal data frames, -1 means default
        int cwMulticast = default(-1); // contention window for broadcast messages, -1 means default

        *.macModule = "^." + macModule;
        *.rxModule = "^." + rxModule;
        *.collisionControllerModule = "^." + collisionControllerModule;

        @class(Dcf);

    submodules:
        contention: Contention;
}
