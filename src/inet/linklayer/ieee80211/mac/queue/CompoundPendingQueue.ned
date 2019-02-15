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

package inet.linklayer.ieee80211.mac.queue;

import inet.common.newqueue.ClassifierBase;
import inet.common.newqueue.CompoundQueue;
import inet.common.newqueue.InfiniteQueue;
import inet.common.newqueue.Prioritizer;

simple PendingQueueClassifier extends ClassifierBase
{
    parameters:
        bool prioritizeMulticast = default(false); // if true, prioritize multicast frames (9.3.2.1 Fundamental access)
        @class(PendingQueueClassifier);
}

module CompoundPendingQueue extends CompoundQueue
{
    parameters:
        frameCapacity = default(100); // -1 means unlimited
    submodules:
        classifier: PendingQueueClassifier {
            parameters:
                @display("p=100,200");
        }
        managementQueue: InfiniteQueue {
            parameters:
                @display("p=300,100");
        }
        multicastQueue: InfiniteQueue {
            parameters:
                @display("p=300,200");
        }
        unicastQueue: InfiniteQueue {
            parameters:
                @display("p=300,300");
        }
        prioritizer: Prioritizer {
            parameters:
                @display("p=500,200");
        }
    connections:
        in --> classifier.in;
        classifier.out++ --> managementQueue.in;
        classifier.out++ --> multicastQueue.in;
        classifier.out++ --> unicastQueue.in;
        managementQueue.out --> prioritizer.in++;
        multicastQueue.out --> prioritizer.in++;
        unicastQueue.out --> prioritizer.in++;
        prioritizer.out --> out;
}

