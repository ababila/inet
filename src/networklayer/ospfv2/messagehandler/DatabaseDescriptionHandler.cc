//
// Copyright (C) 2006 Andras Babos and Andras Varga
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
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "DatabaseDescriptionHandler.h"
#include "OSPFNeighbor.h"
#include "OSPFInterface.h"
#include "OSPFRouter.h"
#include "OSPFArea.h"

OSPF::DatabaseDescriptionHandler::DatabaseDescriptionHandler(OSPF::Router* containingRouter) :
    OSPF::IMessageHandler(containingRouter)
{
}

void OSPF::DatabaseDescriptionHandler::ProcessPacket(OSPFPacket* packet, OSPF::Interface* intf, OSPF::Neighbor* neighbor)
{
    router->getMessageHandler()->PrintEvent("Database Description packet received", intf, neighbor);

    OSPFDatabaseDescriptionPacket* ddPacket = check_and_cast<OSPFDatabaseDescriptionPacket*> (packet);

    OSPF::Neighbor::NeighborStateType neighborState = neighbor->getState();

    if ((ddPacket->getInterfaceMTU() <= intf->getMTU()) &&
        (neighborState > OSPF::Neighbor::AttemptState))
    {
        switch (neighborState) {
            case OSPF::Neighbor::TwoWayState:
                break;
            case OSPF::Neighbor::InitState:
                neighbor->ProcessEvent(OSPF::Neighbor::TwoWayReceived);
                break;
            case OSPF::Neighbor::ExchangeStartState:
                {
                    OSPFDDOptions& ddOptions = ddPacket->getDdOptions();

                    if (ddOptions.I_Init && ddOptions.M_More && ddOptions.MS_MasterSlave &&
                        (ddPacket->getLsaHeadersArraySize() == 0))
                    {
                        if (neighbor->getNeighborID() > router->getRouterID()) {
                            OSPF::Neighbor::DDPacketID packetID;
                            packetID.ddOptions      = ddOptions;
                            packetID.options        = ddPacket->getOptions();
                            packetID.sequenceNumber = ddPacket->getDdSequenceNumber();

                            neighbor->setOptions(packetID.options);
                            neighbor->setDatabaseExchangeRelationship(OSPF::Neighbor::Slave);
                            neighbor->setDDSequenceNumber(packetID.sequenceNumber);
                            neighbor->setLastReceivedDDPacket(packetID);

                            if (!ProcessDDPacket(ddPacket, intf, neighbor, true)) {
                                break;
                            }

                            neighbor->ProcessEvent(OSPF::Neighbor::NegotiationDone);
                            if (!neighbor->IsLinkStateRequestListEmpty() &&
                                !neighbor->IsRequestRetransmissionTimerActive())
                            {
                                neighbor->SendLinkStateRequestPacket();
                                neighbor->ClearRequestRetransmissionTimer();
                                neighbor->StartRequestRetransmissionTimer();
                            }
                        } else {
                            neighbor->SendDatabaseDescriptionPacket(true);
                        }
                    }
                    if (!ddOptions.I_Init && !ddOptions.MS_MasterSlave &&
                        (ddPacket->getDdSequenceNumber() == neighbor->getDDSequenceNumber()) &&
                        (neighbor->getNeighborID() < router->getRouterID()))
                    {
                        OSPF::Neighbor::DDPacketID packetID;
                        packetID.ddOptions      = ddOptions;
                        packetID.options        = ddPacket->getOptions();
                        packetID.sequenceNumber = ddPacket->getDdSequenceNumber();

                        neighbor->setOptions(packetID.options);
                        neighbor->setDatabaseExchangeRelationship(OSPF::Neighbor::Master);
                        neighbor->setLastReceivedDDPacket(packetID);

                        if (!ProcessDDPacket(ddPacket, intf, neighbor, true)) {
                            break;
                        }

                        neighbor->ProcessEvent(OSPF::Neighbor::NegotiationDone);
                        if (!neighbor->IsLinkStateRequestListEmpty() &&
                            !neighbor->IsRequestRetransmissionTimerActive())
                        {
                            neighbor->SendLinkStateRequestPacket();
                            neighbor->ClearRequestRetransmissionTimer();
                            neighbor->StartRequestRetransmissionTimer();
                        }
                    }
                }
                break;
            case OSPF::Neighbor::ExchangeState:
                {
                    OSPF::Neighbor::DDPacketID packetID;
                    packetID.ddOptions      = ddPacket->getDdOptions();
                    packetID.options        = ddPacket->getOptions();
                    packetID.sequenceNumber = ddPacket->getDdSequenceNumber();

                    if (packetID != neighbor->getLastReceivedDDPacket()) {
                        if ((packetID.ddOptions.MS_MasterSlave &&
                             (neighbor->getDatabaseExchangeRelationship() != OSPF::Neighbor::Slave)) ||
                            (!packetID.ddOptions.MS_MasterSlave &&
                             (neighbor->getDatabaseExchangeRelationship() != OSPF::Neighbor::Master)) ||
                            packetID.ddOptions.I_Init ||
                            (packetID.options != neighbor->getLastReceivedDDPacket().options))
                        {
                            neighbor->ProcessEvent(OSPF::Neighbor::SequenceNumberMismatch);
                        } else {
                            if (((neighbor->getDatabaseExchangeRelationship() == OSPF::Neighbor::Master) &&
                                 (packetID.sequenceNumber == neighbor->getDDSequenceNumber())) ||
                                ((neighbor->getDatabaseExchangeRelationship() == OSPF::Neighbor::Slave) &&
                                 (packetID.sequenceNumber == (neighbor->getDDSequenceNumber() + 1))))
                            {
                                neighbor->setLastReceivedDDPacket(packetID);
                                if (!ProcessDDPacket(ddPacket, intf, neighbor, false)) {
                                    break;
                                }
                                if (!neighbor->IsLinkStateRequestListEmpty() &&
                                    !neighbor->IsRequestRetransmissionTimerActive())
                                {
                                    neighbor->SendLinkStateRequestPacket();
                                    neighbor->ClearRequestRetransmissionTimer();
                                    neighbor->StartRequestRetransmissionTimer();
                                }
                            } else {
                                neighbor->ProcessEvent(OSPF::Neighbor::SequenceNumberMismatch);
                            }
                        }
                    } else {
                        if (neighbor->getDatabaseExchangeRelationship() == OSPF::Neighbor::Slave) {
                            neighbor->RetransmitDatabaseDescriptionPacket();
                        }
                    }
                }
                break;
            case OSPF::Neighbor::LoadingState:
            case OSPF::Neighbor::FullState:
                {
                    OSPF::Neighbor::DDPacketID packetID;
                    packetID.ddOptions      = ddPacket->getDdOptions();
                    packetID.options        = ddPacket->getOptions();
                    packetID.sequenceNumber = ddPacket->getDdSequenceNumber();

                    if ((packetID != neighbor->getLastReceivedDDPacket()) ||
                        (packetID.ddOptions.I_Init))
                    {
                        neighbor->ProcessEvent(OSPF::Neighbor::SequenceNumberMismatch);
                    } else {
                        if (neighbor->getDatabaseExchangeRelationship() == OSPF::Neighbor::Slave) {
                            if (!neighbor->RetransmitDatabaseDescriptionPacket()) {
                                neighbor->ProcessEvent(OSPF::Neighbor::SequenceNumberMismatch);
                            }
                        }
                    }
                }
                break;
            default: break;
        }
    }
}

bool OSPF::DatabaseDescriptionHandler::ProcessDDPacket(OSPFDatabaseDescriptionPacket* ddPacket, OSPF::Interface* intf, OSPF::Neighbor* neighbor, bool inExchangeStart)
{
    EV << "  Processing packet contents(ddOptions="
       << ((ddPacket->getDdOptions().I_Init) ? "I " : "_ ")
       << ((ddPacket->getDdOptions().M_More) ? "M " : "_ ")
       << ((ddPacket->getDdOptions().MS_MasterSlave) ? "MS" : "__")
       << "; seqNumber="
       << ddPacket->getDdSequenceNumber()
       << "):\n";

    unsigned int headerCount = ddPacket->getLsaHeadersArraySize();

    for (unsigned int i = 0; i < headerCount; i++) {
        OSPFLSAHeader& currentHeader = ddPacket->getLsaHeaders(i);
        LSAType        lsaType       = static_cast<LSAType> (currentHeader.getLsType());

        EV << "    ";
        PrintLSAHeader(currentHeader, ev.getOStream());

        if ((lsaType < RouterLSAType) || (lsaType > ASExternalLSAType) ||
            ((lsaType == ASExternalLSAType) && (!intf->getArea()->getExternalRoutingCapability())))
        {
            EV << " Error!\n";
            neighbor->ProcessEvent(OSPF::Neighbor::SequenceNumberMismatch);
            return false;
        } else {
            OSPF::LSAKeyType lsaKey;

            lsaKey.linkStateID = currentHeader.getLinkStateID();
            lsaKey.advertisingRouter = currentHeader.getAdvertisingRouter().getInt();

            OSPFLSA* lsaInDatabase = router->FindLSA(lsaType, lsaKey, intf->getArea()->getAreaID());

            // operator< and operator== on OSPFLSAHeaders determines which one is newer(less means older)
            if ((lsaInDatabase == NULL) || (lsaInDatabase->getHeader() < currentHeader)) {
                EV << " (newer)";
                neighbor->AddToRequestList(&currentHeader);
            }
        }
        EV << "\n";
    }

    if (neighbor->getDatabaseExchangeRelationship() == OSPF::Neighbor::Master) {
        neighbor->IncrementDDSequenceNumber();
        if ((neighbor->getDatabaseSummaryListCount() == 0) && !ddPacket->getDdOptions().M_More) {
            neighbor->ProcessEvent(OSPF::Neighbor::ExchangeDone);  // does nothing in ExchangeStart
        } else {
            if (!inExchangeStart) {
                neighbor->SendDatabaseDescriptionPacket();
            }
        }
    } else {
        neighbor->setDDSequenceNumber(ddPacket->getDdSequenceNumber());
        if (!inExchangeStart) {
            neighbor->SendDatabaseDescriptionPacket();
        }
        if (!ddPacket->getDdOptions().M_More &&
            (neighbor->getDatabaseSummaryListCount() == 0))
        {
            neighbor->ProcessEvent(OSPF::Neighbor::ExchangeDone);  // does nothing in ExchangeStart
        }
    }
    return true;
}
