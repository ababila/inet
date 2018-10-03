//
// Copyright (C) OpenSim Ltd.
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

#include "inet/common/newqueue/InfiniteQueue.h"

namespace inet {
namespace queue {

Define_Module(InfiniteQueue);

void InfiniteQueue::initialize(int stage)
{
    PacketQueueBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
        updateDisplayString();
}

int InfiniteQueue::getNumPackets()
{
    return queue.getLength();
}

Packet *InfiniteQueue::getPacket(int index)
{
    return check_and_cast<Packet *>(queue.get(index));
}

void InfiniteQueue::pushPacket(Packet *packet)
{
    EV_INFO << "Pushing packet " << packet->getName() << " into the queue." << endl;
    queue.insert(packet);
    if (hasPendingRequestPacket)
        handlePendingRequestPacket();
    updateDisplayString();
    emit(packetEnqueuedSignal, packet);
}

Packet *InfiniteQueue::popPacket()
{
    auto packet = check_and_cast<Packet *>(queue.pop());
    EV_INFO << "Popping packet " << packet->getName() << " from the queue." << endl;
    updateDisplayString();
    emit(packetDequeuedSignal, packet);
    return packet;
}

void InfiniteQueue::removePacket(Packet *packet)
{
    queue.remove(packet);
    EV_INFO << "Removing packet " << packet->getName() << " from the queue." << endl;
    updateDisplayString();
    emit(packetDequeuedSignal, packet);
}

void InfiniteQueue::updateDisplayString()
{
    std::string text = std::to_string(queue.getLength()) + " packets\n" + std::to_string(queue.getByteLength()) + " bytes";
    getDisplayString().setTagArg("t", 0, text.c_str());
}

} // namespace queue
} // namespace inet
