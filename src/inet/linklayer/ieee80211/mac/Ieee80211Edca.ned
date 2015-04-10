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

simple Ieee80211Edca
{

    bool supportMultipleAccessCategories = default(false);
    // parameters for supportMultipleAccessCategories = true
    string classifier = default("Ieee80211eClassifier");
    int defaultAC = default(0); // the default AC category for frames that cannot be classified
    int AIFSN0 = default(7); // AIFSN for background
    int AIFSN1 = default(3); // AIFSN for best effort
    int AIFSN2 = default(2); // AIFSN for video
    int AIFSN3 = default(2); // AIFSN for voice
    double TXOP0 @unit(s) = default(0s);
    double TXOP1 @unit(s) = default(0s);
    double TXOP2 @unit(s) = default(3.008ms);
    double TXOP3 @unit(s) = default(1.504ms);
    // parameters for supportMultipleAccessCategories = false
    int AIFSN = default(2); // if there is only one AC (EDCA = false)
    // duplicate
    int cwMinData = default(-1); // contention window for normal data frames, -1 means default
    int cwMaxData = default(-1); // contention window for normal data frames, -1 means default
}