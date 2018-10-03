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

#include "inet/common/newqueue/CompoundQueue.h"
#include "inet/common/Simsignals.h"

namespace inet {
namespace queue {

Define_Module(CompoundQueue);

bool CompoundQueue::InputGate::deliver(cMessage *msg, simtime_t at)
{
    bool result = cGate::deliver(msg, at);
    auto compoundQueue = check_and_cast<CompoundQueue *>(getOwnerModule());
    compoundQueue->scheduleHandlePendingRequest();
    return result;
}

cGate *CompoundQueue::createGateObject(cGate::Type type)
{
    if (type == cGate::INPUT)
        return new InputGate();
    else
        return cModule::createGateObject(type);
}

void CompoundQueue::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        frameCapacity = par("frameCapacity");
        inputGate = gate("in");
        outputGate = gate("out");
        inputQueue = check_and_cast<IPacketQueue *>(gate("in")->getPathEndGate()->getOwnerModule());
        outputQueue = check_and_cast<IPacketQueue *>(gate("out")->getPathStartGate()->getOwnerModule());
    }
}

void CompoundQueue::handleMessage(cMessage *msg)
{
    if (msg == &pendingRequestPacket) {
        if (hasPendingRequestPacket)
            handlePendingRequestPacket();
    }
    else
        throw cRuntimeError("Unknown message");
}

void CompoundQueue::handlePendingRequestPacket()
{
    if (!isEmpty()) {
        outputQueue->requestPacket();
        hasPendingRequestPacket = false;
    }
}

void CompoundQueue::scheduleHandlePendingRequest()
{
    scheduleAt(simTime(), &pendingRequestPacket);
}

int CompoundQueue::getNumPackets()
{
    return outputQueue->getNumPackets();
}

Packet *CompoundQueue::getPacket(int index)
{
    return outputQueue->getPacket(index);
}

void CompoundQueue::pushPacket(Packet *packet)
{
    if (frameCapacity != -1 && getNumPackets() >= frameCapacity) {
        EV_INFO << "Dropping packet " << packet->getName() << " because the queue is full." << endl;
        PacketDropDetails details;
        details.setReason(QUEUE_OVERFLOW);
        details.setLimit(frameCapacity);
        emit(packetDroppedSignal, packet, &details);
        delete packet;
    }
    else {
        animateSend(packet, inputGate);
        inputQueue->pushPacket(packet);
        if (hasPendingRequestPacket)
            handlePendingRequestPacket();
    }
}

Packet *CompoundQueue::popPacket()
{
    auto packet = outputQueue->popPacket();
    animateSend(packet, outputGate->getPathStartGate());
    return packet;
}

void CompoundQueue::removePacket(Packet *packet)
{
    outputQueue->removePacket(packet);
}

} // namespace queue
} // namespace inet
