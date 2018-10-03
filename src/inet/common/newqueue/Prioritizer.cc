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

#include "inet/common/newqueue/Prioritizer.h"

namespace inet {
namespace queue {

Define_Module(Prioritizer);

void Prioritizer::initialize()
{
    for (int i = 0; i < gateSize("in"); i++) {
        auto inputGate = gate("in", i);
        auto inputQueue = check_and_cast<IPacketQueue *>(inputGate->getPathStartGate()->getOwnerModule());
        inputGates.push_back(inputGate);
        inputQueues.push_back(inputQueue);
        auto inputQueueModule = check_and_cast<cModule *>(inputQueue);
        inputQueueModule->subscribe(IPacketQueue::packetEnqueuedSignal, this);
    }
}

void Prioritizer::finish()
{
    for (auto inputQueue : inputQueues) {
        auto inputQueueModule = check_and_cast<cModule *>(inputQueue);
        inputQueueModule->unsubscribe(IPacketQueue::packetEnqueuedSignal, this);
    }
}

int Prioritizer::getNumPackets()
{
    int size = 0;
    for (int i = 0; i < gateSize("in"); i++)
        size += inputQueues[i]->getNumPackets();
    return size;
}

Packet *Prioritizer::getPacket(int index)
{
    for (int i = 0; i < gateSize("in"); i++) {
        auto queue = inputQueues[i];
        auto numPackets = queue->getNumPackets();
        if (index < numPackets)
            return queue->getPacket(index);
        else
            index -= numPackets;
    }
    throw cRuntimeError("Invalid operation");
}

void Prioritizer::pushPacket(Packet *packet)
{
    throw cRuntimeError("Invalid operation");
}

Packet *Prioritizer::popPacket()
{
    for (int i = 0; i < gateSize("in"); i++) {
        auto queue = inputQueues[i];
        if (!queue->isEmpty()) {
            auto packet = queue->popPacket();
            animateSend(packet, inputGates[i]->getPathStartGate());
            return packet;
        }
    }
    throw cRuntimeError("Invalid operation");
}

void Prioritizer::removePacket(Packet *packet)
{
    for (int i = 0; i < gateSize("in"); i++) {
        auto queue = inputQueues[i];
        for (int j = 0; j < queue->getNumPackets(); j++) {
            if (queue->getPacket(j) == packet) {
                queue->removePacket(packet);
                return;
            }
        }
    }
    throw cRuntimeError("Invalid operation");
}

void Prioritizer::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    if (signalID == IPacketQueue::packetEnqueuedSignal) {
        if (hasPendingRequestPacket)
            handlePendingRequestPacket();
    }
    else
        throw cRuntimeError("Unknown signal");
}

} // namespace queue
} // namespace inet
