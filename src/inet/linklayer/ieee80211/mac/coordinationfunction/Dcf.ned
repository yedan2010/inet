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

package inet.linklayer.ieee80211.mac.coordinationfunction;

import inet.linklayer.ieee80211.mac.contention.Contention;
import inet.linklayer.ieee80211.mac.originator.OriginatorMacDataService;
import inet.linklayer.ieee80211.mac.originator.OriginatorMpduHandler;
import inet.linklayer.ieee80211.mac.recipient.RecipientMacDataService;
import inet.linklayer.ieee80211.mac.recipient.RecipientMpduHandler;


//
// IEEE 802.11 Distributed Coordination Function
//
module Dcf
{
    parameters:
        double difsTime @unit("s") = default(-1s);

        string rxModule;
        string txModule;
        string rateSelectionModule;
        *.difsTime = difsTime;
        *.rateSelectionModule = "^." + rateSelectionModule;
        *.rxModule = "^." + rxModule;
        *.txModule = "^." + txModule;
        *.originatorMacDataServiceModule = "^.originatorMacDataService";
        *.recipientDataServiceModule = "^.recipientMacDataService";

        @class(Dcf);
        @display("i=block/queue");

    submodules:
        contention: Contention {
            @display("p=210,82");
        }
        // FIXME: We need a module similar to Edca.
        originatorMacDataService: OriginatorMacDataService {
            @display("p=210,153");
        }
        originatorMpduHandler: OriginatorMpduHandler {
            @display("p=210,226;i=block/dispatch");
        }
        recipientMpduHandler: RecipientMpduHandler {
            @display("p=210,371;i=block/dispatch");
        }
        recipientMacDataService: RecipientMacDataService {
            @display("p=210,305");
        }
}
