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

#include "inet/linklayer/ieee80211/mac/Ieee80211Queue.h"

namespace inet {
namespace ieee80211 {

Ieee80211Queue::Ieee80211Queue(int maxQueueSize, const char *name) :
    cQueue(name, nullptr)
{
    this->maxQueueSize = maxQueueSize;
}

void Ieee80211Queue::insert(Ieee80211Frame *frame)
{
    if (getLength() == maxQueueSize)
        throw cRuntimeError("The queue is full");
    cQueue::insert(frame);
}

void Ieee80211Queue::insertBefore(Ieee80211Frame *where, Ieee80211Frame *frame)
{
    if (getLength() == maxQueueSize)
        throw cRuntimeError("The queue is full");
    cQueue::insertBefore(where, frame);
}

void Ieee80211Queue::insertAfter(Ieee80211Frame *where, Ieee80211Frame *frame)
{
    if (getLength() == maxQueueSize)
        throw cRuntimeError("The queue is full");
    cQueue::insertAfter(where, frame);
}

Ieee80211Frame *Ieee80211Queue::remove(Ieee80211Frame *frame)
{
    return check_and_cast<Ieee80211Frame *>(cQueue::remove(frame));
}

Ieee80211Frame *Ieee80211Queue::pop()
{
    return check_and_cast<Ieee80211Frame *>(cQueue::pop());
}

Ieee80211Frame *Ieee80211Queue::front() const
{
    return check_and_cast<Ieee80211Frame *>(cQueue::front());
}

Ieee80211Frame *Ieee80211Queue::back() const
{
    return check_and_cast<Ieee80211Frame *>(cQueue::back());
}

bool Ieee80211Queue::contains(Ieee80211Frame *frame) const
{
    return cQueue::contains(frame);
}

PendingQueue::PendingQueue(int maxQueueSize, const char *name) :
    Ieee80211Queue(maxQueueSize, name)
{
}

PendingQueue::PendingQueue(int maxQueueSize, const char *name, Priority priority) :
    Ieee80211Queue(maxQueueSize, name)
{
    if (priority == Priority::PRIORITIZE_MGMT_OVER_DATA)
        cQueue(name, (CompareFunc)cmpMgmtOverData);
    else if (priority == Priority::PRIORITIZE_MULTICAST_OVER_DATA)
        cQueue(name, (CompareFunc)cmpMgmtOverMulticastOverUnicast);
    else
        throw cRuntimeError("Unknown 802.11 queue priority");
    this->maxQueueSize = maxQueueSize;
}

int PendingQueue::cmpMgmtOverData(Ieee80211DataOrMgmtFrame *a, Ieee80211DataOrMgmtFrame *b)
{
    int aPri = dynamic_cast<Ieee80211ManagementFrame*>(a) ? 1 : 0;  //TODO there should really exist a high-performance isMgmtFrame() function!
    int bPri = dynamic_cast<Ieee80211ManagementFrame*>(b) ? 1 : 0;
    return bPri - aPri;
}

int PendingQueue::cmpMgmtOverMulticastOverUnicast(Ieee80211DataOrMgmtFrame *a, Ieee80211DataOrMgmtFrame *b)
{
    int aPri = dynamic_cast<Ieee80211ManagementFrame*>(a) ? 2 : a->getReceiverAddress().isMulticast() ? 1 : 0;
    int bPri = dynamic_cast<Ieee80211ManagementFrame*>(a) ? 2 : b->getReceiverAddress().isMulticast() ? 1 : 0;
    return bPri - aPri;
}

InProgressQueue::InProgressQueue(FrameExtractor *extractor, int maxQueueSize, const char *name) :
    Ieee80211Queue(maxQueueSize, name),
    extractor(extractor)
{
}

void InProgressQueue::ensureFilled()
{
    if (isEmpty()) {
        auto frames = extractor->extractFramesToTransmit(this);
        for (auto frame : frames)
            insert(frame);
    }
}

Ieee80211Frame *InProgressQueue::pop()
{
    ensureFilled();
    return Ieee80211Queue::pop();
}

Ieee80211Frame *InProgressQueue::front() const
{
    const_cast<InProgressQueue *>(this)->ensureFilled();
    return Ieee80211Queue::front();
}

} /* namespace inet */
} /* namespace ieee80211 */

