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

#ifndef __INET_IORIGINATORMPDUHANDLER_H
#define __INET_IORIGINATORMPDUHANDLER_H

#include "inet/linklayer/ieee80211/mac/framesequence/FrameSequenceContext.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/InProgressFrames.h"
#include "inet/linklayer/ieee80211/mac/originator/AckHandler.h"
#include "inet/linklayer/ieee80211/mac/originator/OriginatorMacDataService.h"
#include "inet/linklayer/ieee80211/mac/originator/RecoveryProcedure.h"

namespace inet {
namespace ieee80211 {

class IOriginatorMpduHandler
{
    public:
        virtual void processRtsProtectionFailed(Ieee80211DataOrMgmtFrame *protectedFrame) = 0;
        virtual void processTransmittedFrame(Ieee80211Frame* transmittedFrame) = 0;
        virtual void processReceivedFrame(Ieee80211Frame *frame, Ieee80211Frame *lastTransmittedFrame) = 0;
        virtual void processFailedFrame(Ieee80211DataOrMgmtFrame* failedFrame) = 0;

        virtual void upperFrameReceived(Ieee80211DataOrMgmtFrame* frame) = 0;
        virtual bool hasFrameToTransmit() = 0;

        virtual FrameSequenceContext *buildContext() = 0;

        // TODO: kludge
        virtual int getCw() = 0;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_IORIGINATORMPDUHANDLER_H
