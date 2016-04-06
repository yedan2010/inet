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

#ifndef __INET_RECEIVEBUFFER_H
#define __INET_RECEIVEBUFFER_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/common/Ieee80211Defs.h"

namespace inet {
namespace ieee80211 {

class INET_API ReceiveBuffer
{
    public:
        typedef std::vector<Ieee80211DataFrame *> Fragments;
        typedef std::map<SequenceNumber, Fragments> ReorderBuffer;

    protected:
        ReorderBuffer buffer;
        // For each Block Ack agreement, the recipient maintains a MAC variable NextExpectedSequenceNumber. The
        // NextExpectedSequenceNumber is initialized to 0 when a Block Ack agreement is accepted.
        int bufferSize = -1;
        int length = 0;
        int nextExpectedSequenceNumber = 0;

    public:
        ReceiveBuffer(int bufferSize);

        bool insertFrame(Ieee80211DataFrame *dataFrame);
        bool remove(int sequenceNumber);

        const ReorderBuffer& getBuffer() { return buffer; }
        int getLength() { return length; }
        int getBufferSize() { return bufferSize; }
        int getNextExpectedSequenceNumber() { return nextExpectedSequenceNumber; }
        void setNextExpectedSequenceNumber(int nextExpectedSequenceNumber) { this->nextExpectedSequenceNumber = nextExpectedSequenceNumber; }
        bool isFull() { ASSERT(length <= bufferSize); return length == bufferSize; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // __INET_RECEIVEBUFFER_H
