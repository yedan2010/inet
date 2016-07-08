//
// Copyright (C) 2016 OpenSim Ltd.
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

#ifndef __INET_EDCA_H
#define __INET_EDCA_H

#include "channelaccess/Edcaf.h"

namespace inet {
namespace ieee80211 {

/**
 * Implements IEEE 802.11 Enhanced Distributed Channel Access.
 */
class INET_API Edca : public cSimpleModule
{
    protected:
        std::vector<Edcaf *> edcafs;
        ICollisionController *collisionController = nullptr;

    protected:
        virtual int numInitStages() const override { return NUM_INIT_STAGES;}
        virtual void initialize(int stage) override;

        virtual Edcaf *getActiveEdcaf();
        virtual AccessCategory classifyFrame(Ieee80211DataOrMgmtFrame *frame);
        virtual AccessCategory mapTidToAc(Tid tid);
        virtual Tid getTid(Ieee80211DataOrMgmtFrame *frame);

    public:
        virtual bool isSequenceRunning() { return getActiveEdcaf() != nullptr; }
        virtual bool isInternalCollision(Edcaf *edcaf) override;

        virtual const std::vector<Edcaf *>& getEdcafs() { return edcafs; }
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_EDCA_H
