//
// Copyright (C) OpenSimLtd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_EXTIEEE80211TAPDEVICEFILEIO_H
#define __INET_EXTIEEE80211TAPDEVICEFILEIO_H

#include "inet/common/packet/printer/PacketPrinter.h"
#include "inet/common/scheduler/RealTimeScheduler.h"

namespace inet {

/**
 * Implements an interface suitable for emulation that corresponds to a real
 * interface on the host running the simulation.
 *
 * Requires RealTimeScheduler to be configured as scheduler in omnetpp.ini.
 *
 * See NED file for more details.
 */
class INET_API ExtIeee80211TapDeviceFileIo : public cSimpleModule, public RealTimeScheduler::ICallback
{
  protected:
    // parameters
    std::string device;
    const char *packetNameFormat = nullptr;
    RealTimeScheduler *rtScheduler = nullptr;

    // statistics
    int numSent = 0;
    int numReceived = 0;

    // state
    PacketPrinter packetPrinter;
    int fd = -1;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    virtual void finish() override;

    virtual void openTap(std::string dev);
    virtual void closeTap();

  public:
    virtual ~ExtIeee80211TapDeviceFileIo();

    virtual bool notify(int fd) override;
};

} // namespace inet

#endif // ifndef __INET_EXTIEEE80211TAPDEVICEFILEIO_H
