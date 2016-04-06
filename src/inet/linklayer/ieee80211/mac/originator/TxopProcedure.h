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

#ifndef __INET_TXOPPROCEDURE_H
#define __INET_TXOPPROCEDURE_H

#include "inet/common/INETDefs.h"

namespace inet {
namespace ieee80211 {

class INET_API TxOpProcedure // FIXME: rename to TxOp
{
    public:
        enum class Type {
            EDCA,
            HCCA,
        };

    protected:
        Type type;
        simtime_t start;
        simtime_t limit;

    public:
        TxOpProcedure(Type type, simtime_t limit) : type(type), start(simTime()), limit(limit)
        { }

        virtual Type getType() const { return type; }
        virtual simtime_t getStart() const { return start; }
        virtual simtime_t getLimit() const { return limit; }
        virtual simtime_t getRemaining() const { auto now = simTime(); return now > start + limit ? 0 : now - start; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_TXOPPROCEDURE_H
