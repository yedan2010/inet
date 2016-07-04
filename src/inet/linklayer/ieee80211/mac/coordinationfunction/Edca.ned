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

import inet.linklayer.ieee80211.mac.contract.ICollisionController;
import inet.linklayer.ieee80211.mac.originator.OriginatorBlockAckAgreementHandler;
import inet.linklayer.ieee80211.mac.originator.OriginatorQoSMacDataService;
import inet.linklayer.ieee80211.mac.recipient.RecipientBlockAckAgreementHandler;
import inet.linklayer.ieee80211.mac.recipient.RecipientQoSMacDataService;
import inet.linklayer.ieee80211.mac.recipient.RecipientQoSMpduHandler;


module Edca
{
    parameters:
        int numFoos = default(4);

        string rxModule;
        string txModule;
        string rateSelectionModule;

        foos[0].accessCategory = default("AC_BE");
        foos[1].accessCategory = default("AC_BK");
        foos[2].accessCategory = default("AC_VO");
        foos[3].accessCategory = default("AC_VI");

        *.originatorMacDataServiceModule = "^.originatorQoSMacDataService";
        *.recipientMacDataServiceModule = "^.recipientQoSMacDataService";
        *.collisionControllerModule = "^.ctrl";
        *.recipientBlockAckAgreementHandler = "^.recipientBlockAckAgreementHandler";
        *.originatorBlockAckAgreementHandlerModule = "^.originatorBlockAckAgreementHandler";
        *.rxModule = "^." + rxModule;
        *.txModule = "^." + txModule;
        *.rateSelectionModule = "^." + rateSelectionModule;

        @class("Edca");

    submodules:
        foos[numFoos]: Foo {
            @display("p=350,139");
        }
        ctrl: <default("CollisionController")> like ICollisionController { // TODO: move to uppermac
            @display("p=350,61");
        }
        recipientQoSMpduHandler: RecipientQoSMpduHandler {
            @display("p=350,212;i=block/dispatch");
        }
        originatorQoSMacDataService: OriginatorQoSMacDataService {
            @display("p=350,387");
        }
        recipientQoSMacDataService: RecipientQoSMacDataService {
            @display("p=350,301");
        }
        originatorBlockAckAgreementHandler: OriginatorBlockAckAgreementHandler {
            @display("p=350,547");
        }
        recipientBlockAckAgreementHandler: RecipientBlockAckAgreementHandler {
            @display("p=350,469");
        }
}
