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

package inet.linklayer.ieee80211.mac;

simple Ieee80211MacAutoRate
{
    parameters:
        string macModule = default("^");

        int autoBitrate @enum(0,1) = default(0); // 1 = ARF Rate, 2 = AARF Rate
        bool forceBitRate = default(false); // if true, the MAC will force the bitrate to the physical layer
        int minTimerTimeout = default(15);
        int timerTimeout = default(minTimerTimeout);
        int minSuccessThreshold = default(10);
        int successThreshold = default(minSuccessThreshold);
        int maxSuccessThreshold = default(60);
        double successCoeff = default(2.0);
        double timerCoeff = default(2.0);
}
