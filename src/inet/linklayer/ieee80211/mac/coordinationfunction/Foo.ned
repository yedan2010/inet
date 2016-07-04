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
// along with this program; if not, see http://www.gnu.org/licenses/.
//
// 

package inet.linklayer.ieee80211.mac.coordinationfunction;

import inet.linklayer.ieee80211.mac.originator.OriginatorQoSMpduHandler;

module Foo // TODO: find a good name
{
    parameters:
        string accessCategory @enum("AC_BK","AC_BE","AC_VI","AC_VO");
        double txopLimit @unit(s) = default(-1s);
        int maxQueueSize = default(100);

        string rateSelectionModule;
        string originatorMacDataServiceModule;
        string originatorBlockAckAgreementHandlerModule;
        string collisionControllerModule;
        string rxModule;
        string txModule;

        originatorQoSMpduHandler.txopLimit = txopLimit;
        originatorQoSMpduHandler.maxQueueSize = maxQueueSize;
        
        *.accessCategory = accessCategory;
        *.originatorBlockAckAgreementHandlerModule = "^." + originatorBlockAckAgreementHandlerModule;
        *.originatorMacDataServiceModule = "^." + originatorMacDataServiceModule;
        *.originatorMpduHandlerModule = "^.originatorQoSMpduHandler";
        *.rxModule = "^." + rxModule;
        *.txModule = "^." + txModule;
        *.collisionControllerModule = "^." + collisionControllerModule;
        *.rateSelectionModule = "^." + rateSelectionModule;
        
    submodules:
        edcaf: Edcaf {
            @display("p=517,84,r,100");
        }
        originatorQoSMpduHandler: OriginatorQoSMpduHandler {
            @display("p=517,226;i=block/dispatch");
        }
}

