/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USo
 *
 * Author: Brian O'Neill <broneill@pdx.edu>
 */

#ifndef WILDFIRE_CLIENT_H
#define WILDFIRE_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"

#include "wildfire-message.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup Wildfire
 * \brief Wildfire protocol client
 *
 */
class WildfireClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  WildfireClient ();
  virtual ~WildfireClient ();
  void ScheduleSubscription (Time dt, Ipv4Address dest);
  void SendSubscription (Ipv4Address dest);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Send (void);

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);

  void  SendAck (Ptr<Socket> socket, Address* dest, uint32_t id);
  void  SendMsg (Ptr<Socket> socket, Address* dest, WildfireMessage* message);
  void  Broadcast ();
  void  SetRemote (Address ip, uint16_t port);
  void  SetRemote (Address addr);

  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet
  EventId m_broadcastEvent;; //!< Event to send the next broadcast packet
  bool m_received = false;
  Time m_broadcast_interval;
  uint32_t m_id = 0;
  uint16_t m_port;   //!< Port on which we listen for incoming packets.

  // wildfire related messages
  std::string *m_key = nullptr; //Key from subscription service
  std::map <uint32_t, WildfireMessage*>* m_messages;

  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callbacks for tracing the packet Rx events
  TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Tx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;

};

} // namespace ns3

#endif /* WILDFIRE_CLIENT_H */
