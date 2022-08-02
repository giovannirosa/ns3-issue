/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author:  Giovanni Rosa (giovanni_rosa4@hotmail.com)
 */
#include "work-server.h"
#include "ns3/address-utils.h"
#include "ns3/address.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket.h"
#include "ns3/trace-source-accessor.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("WorkServer");

NS_OBJECT_ENSURE_REGISTERED(WorkServer);

TypeId WorkServer::GetTypeId(void) {
  static TypeId tid =
      TypeId("ns3::WorkServer")
          .SetParent<Application>()
          .SetGroupName("Applications")
          .AddConstructor<WorkServer>()
          .AddAttribute("Local", "The Address on which to Bind the rx socket.",
                        AddressValue(),
                        MakeAddressAccessor(&WorkServer::m_local),
                        MakeAddressChecker())
          .AddAttribute("Protocol",
                        "The type id of the protocol to use for the rx socket.",
                        TypeIdValue(TcpSocketFactory::GetTypeId()),
                        MakeTypeIdAccessor(&WorkServer::m_tid),
                        MakeTypeIdChecker())
          .AddAttribute(
              "EnableSeqTsSizeHeader",
              "Enable optional header tracing of SeqTsSizeHeader",
              BooleanValue(false),
              MakeBooleanAccessor(&WorkServer::m_enableSeqTsSizeHeader),
              MakeBooleanChecker())
          .AddTraceSource("Rx", "A packet has been received",
                          MakeTraceSourceAccessor(&WorkServer::m_rxTrace),
                          "ns3::Packet::AddressTracedCallback")
          .AddTraceSource(
              "RxWithAddresses", "A packet has been received",
              MakeTraceSourceAccessor(&WorkServer::m_rxTraceWithAddresses),
              "ns3::Packet::TwoAddressTracedCallback")
          .AddTraceSource(
              "RxWithSeqTsSize",
              "A packet with SeqTsSize header has been received",
              MakeTraceSourceAccessor(&WorkServer::m_rxTraceWithSeqTsSize),
              "ns3::WorkServer::SeqTsSizeCallback");
  return tid;
}

WorkServer::WorkServer() {
  NS_LOG_FUNCTION(this);
  m_socket = 0;
  m_totalRx = 0;
}

WorkServer::~WorkServer() { NS_LOG_FUNCTION(this); }

uint64_t WorkServer::GetTotalRx() const {
  NS_LOG_FUNCTION(this);
  return m_totalRx;
}

Ptr<Socket> WorkServer::GetListeningSocket(void) const {
  NS_LOG_FUNCTION(this);
  return m_socket;
}

std::list<Ptr<Socket>> WorkServer::GetAcceptedSockets(void) const {
  NS_LOG_FUNCTION(this);
  return m_socketList;
}

void WorkServer::DoDispose(void) {
  NS_LOG_FUNCTION(this);
  m_socket = 0;
  m_socketList.clear();

  // chain up
  Application::DoDispose();
}

// Application Methods
void WorkServer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO("Starting work packet sink...");
  // Create the socket if not already
  if (!m_socket) {
    m_socket = Socket::CreateSocket(GetNode(), m_tid);
    if (m_socket->Bind(m_local) == -1) {
      NS_FATAL_ERROR("Failed to bind socket");
    }
    m_socket->Listen();
    // m_socket->ShutdownSend();
  }

  if (InetSocketAddress::IsMatchingType(m_local)) {
    m_localPort = InetSocketAddress::ConvertFrom(m_local).GetPort();
  } else if (Inet6SocketAddress::IsMatchingType(m_local)) {
    m_localPort = Inet6SocketAddress::ConvertFrom(m_local).GetPort();
  } else {
    m_localPort = 0;
  }
  m_socket->SetRecvCallback(MakeCallback(&WorkServer::HandleRead, this));
  m_socket->SetRecvPktInfo(true);
  m_socket->SetAcceptCallback(
      MakeCallback(&WorkServer::HandleConnectRequest, this),
      MakeCallback(&WorkServer::HandleAccept, this));
  m_socket->SetCloseCallbacks(MakeCallback(&WorkServer::HandlePeerClose, this),
                              MakeCallback(&WorkServer::HandlePeerError, this));
}

void WorkServer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION(this);
  NS_LOG_INFO("Stopping work packet sink...");
  while (!m_socketList.empty()) // these are accepted sockets, close them
  {
    Ptr<Socket> acceptedSocket = m_socketList.front();
    m_socketList.pop_front();
    acceptedSocket->Close();
  }
  if (m_socket) {
    m_socket->Close();
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
  }
}

void WorkServer::HandleRead(Ptr<Socket> socket) {
  NS_LOG_FUNCTION(this << socket);
  NS_LOG_INFO("Handling read work packet sink...");
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  string newBuffer;
  while ((packet = socket->RecvFrom(from))) {
    if (packet->GetSize() == 0) { // EOF
      break;
    }

    newBuffer.clear();
    uint8_t *buffer = new uint8_t[packet->GetSize()];
    packet->CopyData(buffer, packet->GetSize());
    newBuffer = (char *)buffer;
    size_t endOfMsg = newBuffer.find("]");
    newBuffer = newBuffer.substr(0, endOfMsg + 1).c_str();

    if (InetSocketAddress::IsMatchingType(from)) {
      NS_LOG_INFO("Received packet from "
                  << InetSocketAddress::ConvertFrom(from).GetIpv4()
                  << " with message = " << newBuffer);
    } else {
      NS_LOG_INFO("Received packet from "
                  << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                  << " with message = " << newBuffer);
    }

    m_totalRx += packet->GetSize();
    if (InetSocketAddress::IsMatchingType(from)) {
      NS_LOG_INFO("At time "
                  << Simulator::Now().As(Time::S) << " packet sink received "
                  << packet->GetSize() << " bytes from "
                  << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                  << InetSocketAddress::ConvertFrom(from).GetPort()
                  << " total Rx " << m_totalRx << " bytes");
    } else if (Inet6SocketAddress::IsMatchingType(from)) {
      NS_LOG_INFO("At time "
                  << Simulator::Now().As(Time::S) << " packet sink received "
                  << packet->GetSize() << " bytes from "
                  << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                  << Inet6SocketAddress::ConvertFrom(from).GetPort()
                  << " total Rx " << m_totalRx << " bytes");
    }

    if (!m_rxTrace.IsEmpty() || !m_rxTraceWithAddresses.IsEmpty() ||
        (!m_rxTraceWithSeqTsSize.IsEmpty() && m_enableSeqTsSizeHeader)) {
      Ipv4PacketInfoTag interfaceInfo;
      Ipv6PacketInfoTag interface6Info;
      if (packet->RemovePacketTag(interfaceInfo)) {
        localAddress =
            InetSocketAddress(interfaceInfo.GetAddress(), m_localPort);
      } else if (packet->RemovePacketTag(interface6Info)) {
        localAddress =
            Inet6SocketAddress(interface6Info.GetAddress(), m_localPort);
      } else {
        socket->GetSockName(localAddress);
      }
      m_rxTrace(packet, from);
      m_rxTraceWithAddresses(packet, from, localAddress);

      if (!m_rxTraceWithSeqTsSize.IsEmpty() && m_enableSeqTsSizeHeader) {
        PacketReceived(packet, from, localAddress);
      }
    }
  }
  HandlePacket(newBuffer, socket);
}

void WorkServer::PacketReceived(const Ptr<Packet> &p, const Address &from,
                                const Address &localAddress) {
  SeqTsSizeHeader header;
  Ptr<Packet> buffer;

  auto itBuffer = m_buffer.find(from);
  if (itBuffer == m_buffer.end()) {
    itBuffer = m_buffer.insert(std::make_pair(from, Create<Packet>(0))).first;
  }

  buffer = itBuffer->second;
  buffer->AddAtEnd(p);
  buffer->PeekHeader(header);

  NS_ABORT_IF(header.GetSize() == 0);

  while (buffer->GetSize() >= header.GetSize()) {
    NS_LOG_DEBUG("Removing packet of size " << header.GetSize()
                                            << " from buffer of size "
                                            << buffer->GetSize());
    Ptr<Packet> complete =
        buffer->CreateFragment(0, static_cast<uint32_t>(header.GetSize()));
    buffer->RemoveAtStart(static_cast<uint32_t>(header.GetSize()));

    complete->RemoveHeader(header);

    m_rxTraceWithSeqTsSize(complete, from, localAddress, header);

    if (buffer->GetSize() > header.GetSerializedSize()) {
      buffer->PeekHeader(header);
    } else {
      break;
    }
  }
}

void WorkServer::HandlePeerClose(Ptr<Socket> socket) {
  NS_LOG_FUNCTION(this << socket);
}

void WorkServer::HandlePeerError(Ptr<Socket> socket) {
  NS_LOG_FUNCTION(this << socket);
}

bool WorkServer::HandleConnectRequest(Ptr<Socket> socket, const Address &from) {
  if (InetSocketAddress::IsMatchingType(from)) {
    NS_LOG_FUNCTION(this << socket
                         << InetSocketAddress::ConvertFrom(from).GetIpv4());
  } else {
    NS_LOG_FUNCTION(this << socket
                         << Inet6SocketAddress::ConvertFrom(from).GetIpv6());
  }

  return true;
}

void WorkServer::HandleAccept(Ptr<Socket> s, const Address &from) {
  if (InetSocketAddress::IsMatchingType(from)) {
    NS_LOG_FUNCTION(this << s
                         << InetSocketAddress::ConvertFrom(from).GetIpv4());
  } else {
    NS_LOG_FUNCTION(this << s
                         << Inet6SocketAddress::ConvertFrom(from).GetIpv6());
  }
  s->SetRecvCallback(MakeCallback(&WorkServer::HandleRead, this));
  m_socketList.push_back(s);
}

void WorkServer::HandlePacket(string buffer, Ptr<Socket> socket) {
  NS_LOG_FUNCTION(this << buffer);
  if (!buffer.empty()) {
    buffer = buffer.substr(1);
    buffer = buffer.substr(0, buffer.size() - 1);

    string respStr = buffer.size() > 0 ? "[Accepted]" : "[Refused]";

    Ptr<Packet> packet = Create<Packet>(
        reinterpret_cast<const uint8_t *>(respStr.c_str()), respStr.size());

    socket->Send(packet);
    // NS_LOG_INFO(response);
  }
}

} // Namespace ns3
