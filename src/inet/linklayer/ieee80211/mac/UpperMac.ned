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
//

package inet.linklayer.ieee80211.mac;

import inet.linklayer.ieee80211.mac.contract.IUpperMac;
import inet.linklayer.ieee80211.mac.coordinationfunction.Dcf;
import inet.linklayer.ieee80211.mac.coordinationfunction.Edca;


module UpperMac like IUpperMac
{
    parameters:
        dcf.originatorMpduHandlerModule = "^.originatorMpduHandler";
        originatorMpduHandler.macDataServiceModule = "^.originatorMacDataService";
        recipientMpduHandler.macDataServiceModule = "^.recipientMacDataService";

        edcaf[*].originatorMpduHandlerModule = "^.originatorQoSMpduHandler";
        originatorQoSMpduHandler[*].macDataServiceModule = "^.originatorQoSMacDataService";
        recipientQoSMpduHandler[*].macDataServiceModule = "^.recipientQoSMacDataService";

        string macModule;
        string rxModule;
        string txModule;
        string rateSelectionModule;
        string rateControlModule;
        string statisticsModule;
        *.macModule = "^." + macModule;
        *.upperMacModule = "^." + upperMacModule;
        *.rxModule = "^." + rxModule;
        *.txModule = "^." + txModule;
        *.statisticsModule = "^." + statisticsModule;
        *.rateSelectionModule = "^." + rateSelectionModule;

        @display("i=block/layer");
        @class(UpperMac);

    submodules:
        dcf: Dcf {
            @display("p=162,106");
        }
        edca: Edca {
            @display("p=278,106;i=block/queue");
        }
}
