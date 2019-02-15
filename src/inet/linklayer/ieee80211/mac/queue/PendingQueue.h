//
// Copyright (C) 2016 OpenSim Ltd.
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

#ifndef __INET_PENDINGQUEUE_H
#define __INET_PENDINGQUEUE_H

#include "inet/common/newqueue/ClassifierBase.h"
#include "inet/common/newqueue/DropTailQueue.h"

namespace inet {
namespace ieee80211 {

class INET_API PendingQueue : public inet::queue::DropTailQueue
{
    protected:
        virtual void initialize(int stage) override;

    public:
        static int cmpMgmtOverData(Packet *a, Packet *b);
        static int cmpMgmtOverMulticastOverUnicast(Packet *a, Packet *b);
};

class INET_API PendingQueueClassifier : public inet::queue::ClassifierBase
{
    protected:
        virtual int classifyPacket(Packet *packet) const;
};

} /* namespace ieee80211 */
} /* namespace inet */

#endif // ifndef __INET_PENDINGQUEUE_H
