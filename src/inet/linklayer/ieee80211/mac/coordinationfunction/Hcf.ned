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

package inet.linklayer.ieee80211.mac.coordinationfunction;

import inet.linklayer.ieee80211.mac.channelaccess.Edca;
import inet.linklayer.ieee80211.mac.channelaccess.Hcca;
import inet.linklayer.ieee80211.mac.originator.OriginatorBlockAckAgreementHandler;
import inet.linklayer.ieee80211.mac.originator.OriginatorQoSMacDataService;
import inet.linklayer.ieee80211.mac.originator.RecoveryProcedure;
import inet.linklayer.ieee80211.mac.recipient.RecipientBlockAckAgreementHandler;
import inet.linklayer.ieee80211.mac.recipient.RecipientQoSMacDataService;
import inet.linklayer.ieee80211.mac.recipient.RecipientQoSMpduHandler;


//
// HCF -- Hybrid Coordination Function
//
module Hcf
{
    parameters:
        @class(Hcf);
        @display("i=block/queue");

    submodules:
        // Channel access methods
        edca: Edca {
            @display("p=52,262");
        }
        hcca: Hcca {
            @display("p=52,348");
        }

        // Procedures
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
        recoveryProcedure[4]: RecoveryProcedure {
            @display("p=166,338");
        }
}
