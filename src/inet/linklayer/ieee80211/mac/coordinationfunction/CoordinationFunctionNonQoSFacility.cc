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

#include "CoordinationFunctionNonQoSFacility.h"

namespace inet {
namespace ieee80211 {

void CoordinationFunctionNonQoSFacility::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    if (frame->getType() == ST_DATA_WITH_QOS) {
        EV_INFO << "Non-QoS station received a QoS frame, dropping the frame" << std::endl;
        delete frame;
    }
    else
        dcf->processUpperFrame(frame);
}

void CoordinationFunctionNonQoSFacility::processLowerFrame(Ieee80211Frame* frame)
{
    if (frame->getType() == ST_DATA_WITH_QOS) {
        EV_INFO << "Non-QoS station received a QoS frame, dropping the frame" << std::endl;
        delete frame;
        return;
    }
    if (dcf->isSequenceRunning())
        dcf->processLowerFrame(frame);
    else
        recipientMpduHandler->processReceivedFrame(frame);
}

} /* namespace ieee80211 */
} /* namespace inet */
