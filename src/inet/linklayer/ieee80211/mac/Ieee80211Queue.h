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

#ifndef __INET_IEEE80211QUEUE_H
#define __INET_IEEE80211QUEUE_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
#include "inet/linklayer/ieee80211/mac/FrameExtractor.h"

namespace inet {
namespace ieee80211 {

class INET_API Ieee80211Queue : public cQueue
{
    protected:
        int maxQueueSize = -1;

    public:
        Ieee80211Queue(int maxQueueSize, const char *name);

        virtual void insert(Ieee80211Frame *frame);
        virtual void insertBefore(Ieee80211Frame *where, Ieee80211Frame *frame);
        virtual void insertAfter(Ieee80211Frame *where, Ieee80211Frame *frame);

        virtual Ieee80211Frame *remove(Ieee80211Frame *frame);
        virtual Ieee80211Frame *pop();

        virtual Ieee80211Frame *front() const;
        virtual Ieee80211Frame *back() const;

        virtual bool contains(Ieee80211Frame *frame) const;

        int getNumberOfFrames() { return getLength(); }
        int getMaxQueueSize() { return maxQueueSize; }

};

class PendingQueue : public Ieee80211Queue {
    public:
        enum class Priority {
            PRIORITIZE_MGMT_OVER_DATA,
            PRIORITIZE_MULTICAST_OVER_DATA
        };

    public:
        PendingQueue(int maxQueueSize, const char *name);
        PendingQueue(int maxQueueSize, const char *name, Priority priority);

    public:
        static int cmpMgmtOverData(Ieee80211DataOrMgmtFrame *a, Ieee80211DataOrMgmtFrame *b);
        static int cmpMgmtOverMulticastOverUnicast(Ieee80211DataOrMgmtFrame *a, Ieee80211DataOrMgmtFrame *b);
};

class InProgressQueue : public Ieee80211Queue {
    protected:
        FrameExtractor *extractor = nullptr;

    protected:
        void ensureFilled();

    public:
        InProgressQueue(FrameExtractor *extractor, int maxQueueSize, const char *name);

        virtual Ieee80211Frame *pop();
        virtual Ieee80211Frame *front() const;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_IEEE80211QUEUE_H
