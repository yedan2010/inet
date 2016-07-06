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

#include "CoordinationFunctionQoSFacility.h"

namespace inet {
namespace ieee80211 {

void CoordinationFunctionQoSFacility::processUpperFrame(Ieee80211DataOrMgmtFrame* frame)
{
    if (dynamic_cast<Ieee80211ManagementFrame*>(frame))
        dcf->processUpperFrame(frame);
    else if (auto dataFrame = dynamic_cast<Ieee80211DataFrame*>(frame))
        if (dataFrame->getType() == ST_DATA_WITH_QOS)
            hcf->processUpperFrame(frame);
        else
            dcf->processUpperFrame(frame);
    else
        throw cRuntimeError("Unknown frame type");
}

void CoordinationFunctionQoSFacility::processLowerFrame(Ieee80211Frame* frame)
{
    if (dcf->isSequenceRunning())
        dcf->processLowerFrame(frame);
    else if (hcf->isSequenceRunning())
        hcf->lowerFrameReceived(frame);
    else
        recipientMpduHandler->processReceivedFrame(frame);
}

} /* namespace ieee80211 */
} /* namespace inet */
