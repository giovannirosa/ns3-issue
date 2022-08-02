
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Author:  Giovanni Rosa (giovanni_rosa4@hotmail.com)
 */

#ifndef WORK_DEVICE_ENFORCER_H
#define WORK_DEVICE_ENFORCER_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/traced-callback.h"
#include <string>

using namespace std;

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup applications
 * \defgroup onoff DeviceEnforcer
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
 * \ingroup onoff
 *
 * \brief Generate traffic to a single destination according to an
 *        OnOff pattern.
 *
 * This traffic generator follows an On/Off pattern: after
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 *
 * Note:  When an application is started, the first packet transmission
 * occurs _after_ a delay equal to (packet size/bit rate).  Note also,
 * when an application transitions into an off state in between packet
 * transmissions, the remaining time until when the next transmission
 * would have occurred is cached and is used when the application starts
 * up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
 * If the application is started at time 3 seconds, the first packet
 * transmission will be scheduled for time 5 seconds (3 + 1000/500)
 * and subsequent transmissions at 2 second intervals.  If the above
 * application were instead stopped at time 4 seconds, and restarted at
 * time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
 * because when it was stopped at 4 seconds, there was only 1 second remaining
 * until the originally scheduled transmission, and this time remaining
 * information is cached and used to schedule the next transmission
 * upon restarting.
 *
 * If the underlying socket type supports broadcast, this application
 * will automatically enable the SetAllowBroadcast(true) socket option.
 *
 * If the attribute "EnableSeqTsSizeHeader" is enabled, the application will
 * use some bytes of the payload to store an header with a sequence number,
 * a timestamp, and the size of the packet sent. Support for extracting
 * statistics from this header have been added to \c ns3::PacketSink
 * (enable its "EnableSeqTsSizeHeader" attribute), or users may extract
 * the header via trace sources.  Note that the continuity of the sequence
 * number may be disrupted across On/Off cycles.
 */
class DeviceEnforcer : public Application {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId(void);

  DeviceEnforcer();

  virtual ~DeviceEnforcer();

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes(uint64_t maxBytes);

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket(void) const;

  virtual void DoDispose(void);

  // inherited from Application base class.
  virtual void StartApplication(void); // Called at time specified by Start
  virtual void StopApplication(void);  // Called at time specified by Stop

  // helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelEvents();

  // Event handlers
  /**
   * \brief Start an On period
   */
  void StartSending(string message);
  /**
   * \brief Send a packet
   */
  void SendPacket();

  Ptr<Socket> m_socket;       //!< Associated socket
  Address m_peer;             //!< Peer address
  Address m_local;            //!< Local address to bind to
  bool m_connected;           //!< True if connected
  DataRate m_cbrRate;         //!< Rate that data is generated
  DataRate m_cbrRateFailSafe; //!< Rate that data is generated (check copy)
  uint32_t m_pktSize;         //!< Size of packets
  uint32_t m_residualBits;    //!< Number of generated, but not sent, bits
  Time m_lastStartTime;       //!< Time last packet sent
  uint64_t m_maxBytes;        //!< Limit total number of bytes sent
  uint64_t m_totBytes;        //!< Total bytes sent so far
  EventId m_startStopEvent;   //!< Event id for next start or stop event
  EventId m_sendEvent;        //!< Event id of pending "send packet" event
  TypeId m_tid;               //!< Type of the socket used
  uint32_t m_seq{0};          //!< Sequence
  Ptr<Packet> m_unsentPacket; //!< Unsent packet cached for future attempt
  bool m_enableSeqTsSizeHeader{
      false}; //!< Enable or disable the use of SeqTsSizeHeader

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet>> m_txTrace;

  /// Callbacks for tracing the packet Tx events, includes source and
  /// destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &>
      m_txTraceWithAddresses;

  TracedCallback<Address, Address, string>
      m_traces; //!< Traced Callback: messages

  /// Callback for tracing the packet Tx events, includes source, destination,
  /// the packet sent, and header
  TracedCallback<Ptr<const Packet>, const Address &, const Address &,
                 const SeqTsSizeHeader &>
      m_txTraceWithSeqTsSize;

  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx();
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded(Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed(Ptr<Socket> socket);

  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleRead(Ptr<Socket> socket);
  /**
   * \brief Handle an incoming connection
   * \param socket the incoming connection socket
   * \param from the address the connection is from
   */
  void HandleAccept(Ptr<Socket> socket, const Address &from);
  /**
   * \brief Handle an connection close
   * \param socket the connected socket
   */
  void HandlePeerClose(Ptr<Socket> socket);
  /**
   * \brief Handle an connection error
   * \param socket the connected socket
   */
  void HandlePeerError(Ptr<Socket> socket);
};

} // namespace ns3

#endif /* WORK_DEVICE_ENFORCER_H */
