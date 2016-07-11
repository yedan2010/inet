//
// Copyright (C) 2012 Opensim Ltd.
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
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "inet/networklayer/generic/GenericNetworkProtocol.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MACAddressTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/L3SocketCommand_m.h"
#include "inet/networklayer/contract/generic/GenericNetworkProtocolControlInfo.h"
#include "inet/networklayer/generic/GenericDatagram.h"
#include "inet/networklayer/generic/GenericNetworkProtocolInterfaceData.h"
#include "inet/networklayer/generic/GenericRoute.h"
#include "inet/networklayer/generic/GenericRoutingTable.h"

namespace inet {

Define_Module(GenericNetworkProtocol);

GenericNetworkProtocol::GenericNetworkProtocol() :
        interfaceTable(nullptr),
        routingTable(nullptr),
        arp(nullptr),
        defaultHopLimit(-1),
        numLocalDeliver(0),
        numDropped(0),
        numUnroutable(0),
        numForwarded(0)
{
}

GenericNetworkProtocol::~GenericNetworkProtocol()
{
    for (auto & elem : queuedDatagramsForHooks) {
        delete elem.datagram;
    }
    queuedDatagramsForHooks.clear();
}

void GenericNetworkProtocol::initialize(int stage)
{
    if (stage == INITSTAGE_LOCAL) {
        QueueBase::initialize();

        interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        routingTable = getModuleFromPar<GenericRoutingTable>(par("routingTableModule"), this);
        arp = getModuleFromPar<IARP>(par("arpModule"), this);

        defaultHopLimit = par("hopLimit");
        numLocalDeliver = numDropped = numUnroutable = numForwarded = 0;

        WATCH(numLocalDeliver);
        WATCH(numDropped);
        WATCH(numUnroutable);
        WATCH(numForwarded);
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        registerProtocol(Protocol::gnp, gate("transportOut"));
        registerProtocol(Protocol::gnp, gate("queueOut"));
    }
}

void GenericNetworkProtocol::handleRegisterProtocol(const Protocol& protocol, cGate *gate)
{
    Enter_Method("handleRegisterProtocol");
    mapping.addProtocolMapping(ProtocolGroup::ipprotocol.getProtocolNumber(&protocol), gate->getIndex());
}

void GenericNetworkProtocol::updateDisplayString()
{
    char buf[80] = "";
    if (numForwarded > 0)
        sprintf(buf + strlen(buf), "fwd:%d ", numForwarded);
    if (numLocalDeliver > 0)
        sprintf(buf + strlen(buf), "up:%d ", numLocalDeliver);
    if (numDropped > 0)
        sprintf(buf + strlen(buf), "DROP:%d ", numDropped);
    if (numUnroutable > 0)
        sprintf(buf + strlen(buf), "UNROUTABLE:%d ", numUnroutable);
    getDisplayString().setTagArg("t", 0, buf);
}

void GenericNetworkProtocol::handleMessage(cMessage *msg)
{
    if (L3SocketBindCommand *command = dynamic_cast<L3SocketBindCommand *>(msg->getControlInfo())) {
        SocketDescriptor *descriptor = new SocketDescriptor(command->getSocketId(), command->getProtocolId());
        socketIdToSocketDescriptor[command->getSocketId()] = descriptor;
        protocolIdToSocketDescriptors.insert(std::pair<int, SocketDescriptor *>(command->getProtocolId(), descriptor));
        delete msg;
    }
    else if (L3SocketCloseCommand *command = dynamic_cast<L3SocketCloseCommand *>(msg->getControlInfo())) {
        auto it = socketIdToSocketDescriptor.find(command->getSocketId());
        if (it != socketIdToSocketDescriptor.end()) {
            int protocol = it->second->protocolId;
            auto lowerBound = protocolIdToSocketDescriptors.lower_bound(protocol);
            auto upperBound = protocolIdToSocketDescriptors.upper_bound(protocol);
            for (auto jt = lowerBound; jt != upperBound; jt++) {
                if (it->second == jt->second) {
                    protocolIdToSocketDescriptors.erase(jt);
                    break;
                }
            }
            delete it->second;
            socketIdToSocketDescriptor.erase(it);
        }
        delete msg;
    }
    else
        QueueBase::handleMessage(msg);
}

void GenericNetworkProtocol::endService(cPacket *pk)
{
    if (pk->getArrivalGate()->isName("transportIn"))
        handleMessageFromHL(pk);
    else {
        GenericDatagram *dgram = check_and_cast<GenericDatagram *>(pk);
        handlePacketFromNetwork(dgram);
    }

    if (hasGUI())
        updateDisplayString();
}

const InterfaceEntry *GenericNetworkProtocol::getSourceInterfaceFrom(cPacket *packet)
{
    auto interfaceInd = packet->getTag<InterfaceInd>();
    return interfaceInd != nullptr ? interfaceTable->getInterfaceById(interfaceInd->getInterfaceId()) : nullptr;
}

void GenericNetworkProtocol::handlePacketFromNetwork(GenericDatagram *datagram)
{
    if (datagram->hasBitError()) {
        //TODO discard
    }

    // hop counter decrement; FIXME but not if it will be locally delivered
    datagram->setHopLimit(datagram->getHopLimit() - 1);

    L3Address nextHop;
    const InterfaceEntry *inIE = getSourceInterfaceFrom(datagram);

    const InterfaceEntry *destIE = nullptr;
    if (datagramPreRoutingHook(datagram, inIE, destIE, nextHop) != IHook::ACCEPT)
        return;

    datagramPreRouting(datagram, inIE, destIE, nextHop);
}

void GenericNetworkProtocol::handleMessageFromHL(cPacket *msg)
{
    // if no interface exists, do not send datagram
    if (interfaceTable->getNumInterfaces() == 0) {
        EV_INFO << "No interfaces exist, dropping packet\n";
        delete msg;
        return;
    }

    // encapsulate and send
    const InterfaceEntry *destIE;    // will be filled in by encapsulate()
    GenericDatagram *datagram = encapsulate(msg, destIE);

    L3Address nextHop;
    if (datagramLocalOutHook(datagram, destIE, nextHop) != IHook::ACCEPT)
        return;

    datagramLocalOut(datagram, destIE, nextHop);
}

void GenericNetworkProtocol::routePacket(GenericDatagram *datagram, const InterfaceEntry *destIE, const L3Address& requestedNextHop, bool fromHL)
{
    // TBD add option handling code here

    L3Address destAddr = datagram->getDestinationAddress();

    EV_INFO << "Routing datagram `" << datagram->getName() << "' with dest=" << destAddr << ": ";

    // check for local delivery
    if (routingTable->isLocalAddress(destAddr)) {
        EV_INFO << "local delivery\n";
        if (datagram->getSourceAddress().isUnspecified())
            datagram->setSourceAddress(destAddr); // allows two apps on the same host to communicate
        numLocalDeliver++;

        if (datagramLocalInHook(datagram, getSourceInterfaceFrom(datagram)) != INetfilter::IHook::ACCEPT)
            return;

        sendDatagramToHL(datagram);
        return;
    }

    // if datagram arrived from input gate and Generic_FORWARD is off, delete datagram
    if (!fromHL && !routingTable->isForwardingEnabled()) {
        EV_INFO << "forwarding off, dropping packet\n";
        numDropped++;
        delete datagram;
        return;
    }

    // if output port was explicitly requested, use that, otherwise use GenericNetworkProtocol routing
    // TODO: see IPv4, using destIE here leaves nextHope unspecified
    L3Address nextHop;
    if (destIE && !requestedNextHop.isUnspecified()) {
        EV_DETAIL << "using manually specified output interface " << destIE->getName() << "\n";
        nextHop = requestedNextHop;
    }
    else {
        // use GenericNetworkProtocol routing (lookup in routing table)
        const GenericRoute *re = routingTable->findBestMatchingRoute(destAddr);

        // error handling: destination address does not exist in routing table:
        // throw packet away and continue
        if (re == nullptr) {
            EV_INFO << "unroutable, discarding packet\n";
            numUnroutable++;
            delete datagram;
            return;
        }

        // extract interface and next-hop address from routing table entry
        destIE = re->getInterface();
        nextHop = re->getNextHopAsGeneric();
    }

    // set datagram source address if not yet set
    if (datagram->getSourceAddress().isUnspecified())
        datagram->setSourceAddress(destIE->getGenericNetworkProtocolData()->getAddress());

    // default: send datagram to fragmentation
    EV_INFO << "output interface is " << destIE->getName() << ", next-hop address: " << nextHop << "\n";
    numForwarded++;

    sendDatagramToOutput(datagram, destIE, nextHop);
}

void GenericNetworkProtocol::routeMulticastPacket(GenericDatagram *datagram, const InterfaceEntry *destIE, const InterfaceEntry *fromIE)
{
    L3Address destAddr = datagram->getDestinationAddress();
    // if received from the network...
    if (fromIE != nullptr) {
        // check for local delivery
        if (routingTable->isLocalMulticastAddress(destAddr))
            sendDatagramToHL(datagram);
//
//        // don't forward if GenericNetworkProtocol forwarding is off
//        if (!rt->isGenericForwardingEnabled())
//        {
//            delete datagram;
//            return;
//        }
//
//        // don't forward if dest address is link-scope
//        if (destAddr.isLinkLocalMulticast())
//        {
//            delete datagram;
//            return;
//        }
    }
    else {
        //TODO
        for (int i = 0; i < interfaceTable->getNumInterfaces(); ++i) {
            const InterfaceEntry *destIE = interfaceTable->getInterface(i);
            if (!destIE->isLoopback())
                sendDatagramToOutput(datagram->dup(), destIE, datagram->getDestinationAddress());
        }
        delete datagram;
    }

//    Address destAddr = datagram->getDestinationAddress();
//    EV << "Routing multicast datagram `" << datagram->getName() << "' with dest=" << destAddr << "\n";
//
//    numMulticast++;
//
//    // DVMRP: process datagram only if sent locally or arrived on the shortest
//    // route (provided routing table already contains srcAddr); otherwise
//    // discard and continue.
//    const InterfaceEntry *shortestPathIE = rt->getInterfaceForDestinationAddr(datagram->getSourceAddress());
//    if (fromIE!=nullptr && shortestPathIE!=nullptr && fromIE!=shortestPathIE)
//    {
//        // FIXME count dropped
//        EV << "Packet dropped.\n";
//        delete datagram;
//        return;
//    }
//
//    // if received from the network...
//    if (fromIE!=nullptr)
//    {
//        // check for local delivery
//        if (rt->isLocalMulticastAddress(destAddr))
//        {
//            GenericDatagram *datagramCopy = (GenericDatagram *) datagram->dup();
//
//            // FIXME code from the MPLS model: set packet dest address to routerId (???)
//            datagramCopy->setDestinationAddress(rt->getRouterId());
//
//            reassembleAndDeliver(datagramCopy);
//        }
//
//        // don't forward if GenericNetworkProtocol forwarding is off
//        if (!rt->isGenericForwardingEnabled())
//        {
//            delete datagram;
//            return;
//        }
//
//        // don't forward if dest address is link-scope
//        if (destAddr.isLinkLocalMulticast())
//        {
//            delete datagram;
//            return;
//        }
//
//    }
//
//    // routed explicitly via Generic_MULTICAST_IF
//    if (destIE!=nullptr)
//    {
//        ASSERT(datagram->getDestinationAddress().isMulticast());
//
//        EV << "multicast packet explicitly routed via output interface " << destIE->getName() << endl;
//
//        // set datagram source address if not yet set
//        if (datagram->getSourceAddress().isUnspecified())
//            datagram->setSourceAddress(destIE->ipv4Data()->getGenericAddress());
//
//        // send
//        sendDatagramToOutput(datagram, destIE, datagram->getDestinationAddress());
//
//        return;
//    }
//
//    // now: routing
//    MulticastRoutes routes = rt->getMulticastRoutesFor(destAddr);
//    if (routes.size()==0)
//    {
//        // no destination: delete datagram
//        delete datagram;
//    }
//    else
//    {
//        // copy original datagram for multiple destinations
//        for (unsigned int i=0; i<routes.size(); i++)
//        {
//            const InterfaceEntry *destIE = routes[i].interf;
//
//            // don't forward to input port
//            if (destIE && destIE!=fromIE)
//            {
//                GenericDatagram *datagramCopy = (GenericDatagram *) datagram->dup();
//
//                // set datagram source address if not yet set
//                if (datagramCopy->getSourceAddress().isUnspecified())
//                    datagramCopy->setSourceAddress(destIE->ipv4Data()->getGenericAddress());
//
//                // send
//                Address nextHop = routes[i].gateway;
//                sendDatagramToOutput(datagramCopy, destIE, nextHop);
//            }
//        }
//
//        // only copies sent, delete original datagram
//        delete datagram;
//    }
}

cPacket *GenericNetworkProtocol::decapsulate(GenericDatagram *datagram)
{
    // decapsulate transport packet
    const InterfaceEntry *fromIE = getSourceInterfaceFrom(datagram);
    cPacket *packet = datagram->decapsulate();

    // create and fill in control info
    GenericNetworkProtocolControlInfo *controlInfo = new GenericNetworkProtocolControlInfo();
    controlInfo->setProtocol(datagram->getTransportProtocol());
    if (fromIE) {
        auto ifTag = packet->ensureTag<InterfaceInd>();
        ifTag->setInterfaceId(fromIE->getInterfaceId());
    }

    controlInfo->setHopLimit(datagram->getHopLimit());

    // attach control info
    packet->setControlInfo(controlInfo);
    packet->ensureTag<ProtocolReq>()->setProtocol(ProtocolGroup::ipprotocol.getProtocol(datagram->getTransportProtocol()));
    auto l3AddressInd = packet->ensureTag<L3AddressInd>();
    l3AddressInd->setSource(datagram->getSourceAddress());
    l3AddressInd->setDestination(datagram->getDestinationAddress());

    return packet;
}

GenericDatagram *GenericNetworkProtocol::encapsulate(cPacket *transportPacket, const InterfaceEntry *& destIE)
{
    GenericNetworkProtocolControlInfo *controlInfo = check_and_cast<GenericNetworkProtocolControlInfo *>(transportPacket->removeControlInfo());
    GenericDatagram *datagram = new GenericDatagram(transportPacket->getName());
//    datagram->setByteLength(HEADER_BYTES); //TODO parameter
    datagram->encapsulate(transportPacket);

    // set source and destination address
    auto l3AddressReq = transportPacket->removeMandatoryTag<L3AddressReq>();
    L3Address src = l3AddressReq->getSource();
    L3Address dest = l3AddressReq->getDestination();
    delete l3AddressReq;
    datagram->setDestinationAddress(dest);

    // Generic_MULTICAST_IF option, but allow interface selection for unicast packets as well
    auto ifTag = transportPacket->getTag<InterfaceReq>();
    destIE = ifTag ? interfaceTable->getInterfaceById(ifTag->getInterfaceId()) : nullptr;


    // when source address was given, use it; otherwise it'll get the address
    // of the outgoing interface after routing
    if (!src.isUnspecified()) {
        // if interface parameter does not match existing interface, do not send datagram
        if (routingTable->getInterfaceByAddress(src) == nullptr)
            throw cRuntimeError("Wrong source address %s in (%s)%s: no interface with such address",
                    src.str().c_str(), transportPacket->getClassName(), transportPacket->getFullName());
        datagram->setSourceAddress(src);
    }

    // set other fields
    short ttl;
    if (controlInfo->getHopLimit() > 0)
        ttl = controlInfo->getHopLimit();
    else if (false) //TODO: datagram->getDestinationAddress().isLinkLocalMulticast())
        ttl = 1;
    else
        ttl = defaultHopLimit;

    datagram->setHopLimit(ttl);
    datagram->setTransportProtocol(controlInfo->getProtocol());

    // setting GenericNetworkProtocol options is currently not supported

    delete controlInfo;
    return datagram;
}

void GenericNetworkProtocol::sendDatagramToHL(GenericDatagram *datagram)
{
    int protocol = datagram->getTransportProtocol();
    cPacket *packet = decapsulate(datagram);
    // deliver to sockets
    auto lowerBound = protocolIdToSocketDescriptors.lower_bound(protocol);
    auto upperBound = protocolIdToSocketDescriptors.upper_bound(protocol);
    bool hasSocket = lowerBound != upperBound;
    GenericNetworkProtocolControlInfo *controlInfo = check_and_cast<GenericNetworkProtocolControlInfo *>(packet->getControlInfo());
    for (auto it = lowerBound; it != upperBound; it++) {
        GenericNetworkProtocolControlInfo *controlInfoCopy = controlInfo->dup();
        controlInfoCopy->setSocketId(it->second->socketId);
        cPacket *packetCopy = packet->dup();
        packetCopy->setControlInfo(controlInfoCopy);
        send(packetCopy, "transportOut");
    }
    if (mapping.findOutputGateForProtocol(protocol) >= 0) {
        send(packet, "transportOut");
        numLocalDeliver++;
    }
    else if (!hasSocket) {
        EV_ERROR << "Transport protocol ID=" << protocol << " not connected, discarding packet\n";
        //TODO send an ICMP error: protocol unreachable
        // sendToIcmp(datagram, inputInterfaceId, ICMP_DESTINATION_UNREACHABLE, ICMP_DU_PROTOCOL_UNREACHABLE);
        delete packet;
    }
    delete datagram;
}

void GenericNetworkProtocol::sendDatagramToOutput(GenericDatagram *datagram, const InterfaceEntry *ie, L3Address nextHop)
{
    delete datagram->removeControlInfo();

    if (datagram->getByteLength() > ie->getMTU())
        throw cRuntimeError("datagram too large"); //TODO refine

    // hop counter check
    if (datagram->getHopLimit() <= 0) {
        EV_INFO << "datagram hopLimit reached zero, discarding\n";
        delete datagram;    //TODO stats counter???
        return;
    }

    if (!ie->isBroadcast()) {
        EV_DETAIL << "output interface " << ie->getName() << " is not broadcast, skipping ARP\n";
        Ieee802Ctrl *controlInfo = new Ieee802Ctrl();
        controlInfo->setEtherType(ETHERTYPE_INET_GENERIC);
        //Peer to peer interface, no broadcast, no MACAddress. // packet->ensureTag<MACAddressReq>()->setDestinationAddress(macAddress);
        datagram->removeTag<ProtocolReq>();         // send to NIC
        datagram->ensureTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
        datagram->ensureTag<ProtocolInd>()->setProtocol(&Protocol::gnp);
        datagram->setControlInfo(controlInfo);
        send(datagram, "queueOut");
        return;
    }

    // determine what address to look up in ARP cache
    if (nextHop.isUnspecified()) {
        nextHop = datagram->getDestinationAddress();
        EV_WARN << "no next-hop address, using destination address " << nextHop << " (proxy ARP)\n";
    }

    // send out datagram to NIC, with control info attached
    MACAddress nextHopMAC = arp->resolveL3Address(nextHop, ie);
    if (nextHopMAC == MACAddress::UNSPECIFIED_ADDRESS) {
        throw cRuntimeError("ARP couldn't resolve the '%s' address", nextHop.str().c_str());
    }
    else {
        // add control info with MAC address
        Ieee802Ctrl *controlInfo = new Ieee802Ctrl();
        controlInfo->setEtherType(ETHERTYPE_INET_GENERIC);
        datagram->ensureTag<MACAddressReq>()->setDestinationAddress(nextHopMAC);
        datagram->removeTag<ProtocolReq>();         // send to NIC
        datagram->ensureTag<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
        datagram->ensureTag<ProtocolInd>()->setProtocol(&Protocol::gnp);
        datagram->setControlInfo(controlInfo);

        // send out
        send(datagram, "queueOut");
    }
}

void GenericNetworkProtocol::datagramPreRouting(GenericDatagram *datagram, const InterfaceEntry *inIE, const InterfaceEntry *destIE, const L3Address& nextHop)
{
    // route packet
    if (!datagram->getDestinationAddress().isMulticast())
        routePacket(datagram, destIE, nextHop, false);
    else
        routeMulticastPacket(datagram, destIE, inIE);
}

void GenericNetworkProtocol::datagramLocalIn(GenericDatagram *datagram, const InterfaceEntry *inIE)
{
    sendDatagramToHL(datagram);
}

void GenericNetworkProtocol::datagramLocalOut(GenericDatagram *datagram, const InterfaceEntry *destIE, const L3Address& nextHop)
{
    // route packet
    if (!datagram->getDestinationAddress().isMulticast())
        routePacket(datagram, destIE, nextHop, true);
    else
        routeMulticastPacket(datagram, destIE, nullptr);
}

void GenericNetworkProtocol::registerHook(int priority, IHook *hook)
{
    Enter_Method("registerHook()");
    NetfilterBase::registerHook(priority, hook);
}

void GenericNetworkProtocol::unregisterHook(IHook *hook)
{
    Enter_Method("unregisterHook()");
    NetfilterBase::unregisterHook(hook);
}

void GenericNetworkProtocol::dropQueuedDatagram(const INetworkDatagram *datagram)
{
    Enter_Method("dropDatagram()");
    for (auto iter = queuedDatagramsForHooks.begin(); iter != queuedDatagramsForHooks.end(); iter++) {
        if (iter->datagram == datagram) {
            delete datagram;
            queuedDatagramsForHooks.erase(iter);
            return;
        }
    }
}

void GenericNetworkProtocol::reinjectQueuedDatagram(const INetworkDatagram *datagram)
{
    Enter_Method("reinjectDatagram()");
    for (auto iter = queuedDatagramsForHooks.begin(); iter != queuedDatagramsForHooks.end(); iter++) {
        if (iter->datagram == datagram) {
            GenericDatagram *datagram = iter->datagram;
            const InterfaceEntry *inIE = iter->inIE;
            const InterfaceEntry *outIE = iter->outIE;
            const L3Address& nextHop = iter->nextHop;
            INetfilter::IHook::Type hookType = iter->hookType;
            switch (hookType) {
                case INetfilter::IHook::PREROUTING:
                    datagramPreRouting(datagram, inIE, outIE, nextHop);
                    break;

                case INetfilter::IHook::LOCALIN:
                    datagramLocalIn(datagram, inIE);
                    break;

                case INetfilter::IHook::LOCALOUT:
                    datagramLocalOut(datagram, outIE, nextHop);
                    break;

                default:
                    throw cRuntimeError("Re-injection of datagram queued for this hook not implemented");
                    break;
            }
            queuedDatagramsForHooks.erase(iter);
            return;
        }
    }
}

INetfilter::IHook::Result GenericNetworkProtocol::datagramPreRoutingHook(GenericDatagram *datagram, const InterfaceEntry *inIE, const InterfaceEntry *& outIE, L3Address& nextHop)
{
    for (auto & elem : hooks) {
        IHook::Result r = elem.second->datagramPreRoutingHook(datagram, inIE, outIE, nextHop);
        switch (r) {
            case IHook::ACCEPT:
                break;    // continue iteration

            case IHook::DROP:
                delete datagram;
                return r;

            case IHook::QUEUE:
                if (datagram->getOwner() != this)
                    throw cRuntimeError("Model error: netfilter hook changed the owner of queued datagram '%s'", datagram->getFullName());
                queuedDatagramsForHooks.push_back(QueuedDatagramForHook(datagram, inIE, nullptr, nextHop, INetfilter::IHook::PREROUTING));
                return r;

            case IHook::STOLEN:
                return r;

            default:
                throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return IHook::ACCEPT;
}

INetfilter::IHook::Result GenericNetworkProtocol::datagramForwardHook(GenericDatagram *datagram, const InterfaceEntry *inIE, const InterfaceEntry *& outIE, L3Address& nextHop)
{
    for (auto & elem : hooks) {
        IHook::Result r = elem.second->datagramForwardHook(datagram, inIE, outIE, nextHop);
        switch (r) {
            case IHook::ACCEPT:
                break;    // continue iteration

            case IHook::DROP:
                delete datagram;
                return r;

            case IHook::QUEUE:
                queuedDatagramsForHooks.push_back(QueuedDatagramForHook(datagram, inIE, outIE, nextHop, INetfilter::IHook::FORWARD));
                return r;

            case IHook::STOLEN:
                return r;

            default:
                throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return IHook::ACCEPT;
}

INetfilter::IHook::Result GenericNetworkProtocol::datagramPostRoutingHook(GenericDatagram *datagram, const InterfaceEntry *inIE, const InterfaceEntry *& outIE, L3Address& nextHop)
{
    for (auto & elem : hooks) {
        IHook::Result r = elem.second->datagramPostRoutingHook(datagram, inIE, outIE, nextHop);
        switch (r) {
            case IHook::ACCEPT:
                break;    // continue iteration

            case IHook::DROP:
                delete datagram;
                return r;

            case IHook::QUEUE:
                queuedDatagramsForHooks.push_back(QueuedDatagramForHook(datagram, inIE, outIE, nextHop, INetfilter::IHook::POSTROUTING));
                return r;

            case IHook::STOLEN:
                return r;

            default:
                throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return IHook::ACCEPT;
}

INetfilter::IHook::Result GenericNetworkProtocol::datagramLocalInHook(GenericDatagram *datagram, const InterfaceEntry *inIE)
{
    L3Address address;
    for (auto & elem : hooks) {
        IHook::Result r = elem.second->datagramLocalInHook(datagram, inIE);
        switch (r) {
            case IHook::ACCEPT:
                break;    // continue iteration

            case IHook::DROP:
                delete datagram;
                return r;

            case IHook::QUEUE:
                queuedDatagramsForHooks.push_back(QueuedDatagramForHook(datagram, inIE, nullptr, address, INetfilter::IHook::LOCALIN));
                return r;

            case IHook::STOLEN:
                return r;

            default:
                throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return IHook::ACCEPT;
}

INetfilter::IHook::Result GenericNetworkProtocol::datagramLocalOutHook(GenericDatagram *datagram, const InterfaceEntry *& outIE, L3Address& nextHop)
{
    for (auto & elem : hooks) {
        IHook::Result r = elem.second->datagramLocalOutHook(datagram, outIE, nextHop);
        switch (r) {
            case IHook::ACCEPT:
                break;    // continue iteration

            case IHook::DROP:
                delete datagram;
                return r;

            case IHook::QUEUE:
                queuedDatagramsForHooks.push_back(QueuedDatagramForHook(datagram, nullptr, outIE, nextHop, INetfilter::IHook::LOCALOUT));
                return r;

            case IHook::STOLEN:
                return r;

            default:
                throw cRuntimeError("Unknown Hook::Result value: %d", (int)r);
        }
    }
    return IHook::ACCEPT;
}

} // namespace inet

