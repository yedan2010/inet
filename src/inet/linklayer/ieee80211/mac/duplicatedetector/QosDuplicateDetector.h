//
// Copyright (C) 2015 Andras Varga
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
// Author: Andras Varga
//

#ifndef __INET_QOSDUPLICATEDETECTOR_H
#define __INET_QOSDUPLICATEDETECTOR_H

#include <map>

#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ieee80211/mac/common/SequenceControlField.h"
#include "inet/linklayer/ieee80211/mac/contract/IDuplicateDetector.h"

namespace inet {
namespace ieee80211 {
//
// TODO: Separate sequence number assignment and duplicate detection by
// creating DuplicateRemoval and SequenceNumberAssigment modules.
//
class INET_API QoSDuplicateDetector : public IDuplicateDetector
{
    private:
        enum CacheType
        {
            SHARED,
            TIME_PRIORITY,
            DATA
        };

    protected:
        typedef std::pair<MACAddress, Tid> Key;
        typedef std::map<Key, SequenceControlField> Key2SeqValMap;
        typedef std::map<MACAddress, SequenceControlField> Mac2SeqValMap;
        Key2SeqValMap lastSeenSeqNumCache;// cache of last seen sequence numbers per TA
        Mac2SeqValMap lastSeenSharedSeqNumCache;
        Mac2SeqValMap lastSeenTimePriorityManagementSeqNumCache;

        std::map<Key, SequenceNumber> lastSentSeqNums; // last sent sequence numbers per RA
        std::map<MACAddress, SequenceNumber> lastSentTimePrioritySeqNums; // last sent sequence numbers per RA

        std::map<MACAddress, SequenceNumber> lastSentSharedSeqNums; // last sent sequence numbers per RA
        SequenceNumber lastSentSharedCounterSeqNum = 0;

    protected:
        CacheType getCacheType(Ieee80211DataOrMgmtFrame *frame, bool incoming);

    public:
        virtual void assignSequenceNumber(Ieee80211DataOrMgmtFrame *frame) override;
        virtual bool isDuplicate(Ieee80211DataOrMgmtFrame *frame) override;
};

} // namespace ieee80211
} // namespace inet

#endif

