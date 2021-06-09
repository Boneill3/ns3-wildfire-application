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
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "wildfire-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WildfireClientApplication");

NS_OBJECT_ENSURE_REGISTERED (WildfireClient);

TypeId
WildfireClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WildfireClient")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<WildfireClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&WildfireClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WildfireClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&WildfireClient::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BroadcastInterval",
                   "The time to wait between broadcasts",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&WildfireClient::m_broadcast_interval),
                   MakeTimeChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&WildfireClient::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&WildfireClient::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&WildfireClient::m_txTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&WildfireClient::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

WildfireClient::WildfireClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_messages = new std::map<u_int32_t, WildfireMessage*>();
  m_id = rand() % UINT32_MAX;
}

WildfireClient::~WildfireClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  for(std::map<uint32_t, WildfireMessage*>::iterator itr = m_messages->begin (); itr != m_messages->end (); itr++)
    {
      delete (itr->second);
    }
  delete m_messages;
}

void
WildfireClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
WildfireClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
WildfireClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
WildfireClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
        {
          // Listen for any connection on wildfire port
          InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
          if (m_socket->Bind (local) == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          
          // TODO: Move connect to subscribe function
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&WildfireClient::HandleRead, this));
  m_socket->SetAllowBroadcast (true);
}

void
WildfireClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_broadcastEvent);
}

void
WildfireClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);

      // Get data from packet
      uint32_t size = packet->GetSize ();
      uint8_t buffer[size];
      packet->CopyData (buffer, size);

      // Store data as application message
      std::vector<uint8_t> vbuffer (buffer, buffer + sizeof buffer / sizeof buffer[0]);
      WildfireMessage* message = new WildfireMessage (&vbuffer);
      m_messages->insert (std::pair<uint32_t, WildfireMessage*> (message->getId (), message));

      auto converted_message = message->toString ();
      NS_LOG_INFO ("Received message: " << *converted_message);
      delete converted_message;

      if(message->getType () == WildfireMessageType::acknowledgement)
        {
          // TODO: Need to ensure this is ack from subscription request
          if(from == m_peerAddress)
            {
              if(m_key != nullptr)
                {
                  delete m_key;
                }

              m_key = message->getMessage ();
            }
          NS_LOG_INFO ("Ack received on client");
        }

      if(!m_received && message->getType () == WildfireMessageType::notification
         && message->isValid (m_key) && !message->isExpired ())
        {
          m_received = true;
          NS_LOG_INFO ("Send Ack to " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());
          SendAck (socket, &from, message->getId ());
          Broadcast ();
        }
    }
}

void
WildfireClient::Broadcast ()
{
  Ptr<Packet> packet;
  bool found = false;
  NS_LOG_DEBUG ("At time " << Simulator::Now ().As (Time::S) << " Rebroadcast Over Wifi");
  for(auto itr = m_messages->begin (); itr != m_messages->end (); itr++)
    {
      if (itr->second->getType () == WildfireMessageType::notification
          && !itr->second->isExpired ())
        {
          Address dest = InetSocketAddress (Ipv4Address ("255.255.255.255"), m_port);
          SendMsg (m_socket, &dest, itr->second);
        }
      found = true;
    }

  if (found)
    {
      m_broadcastEvent =  Simulator::Schedule (m_broadcast_interval, &WildfireClient::Broadcast, this);
    }

}

void
WildfireClient::SendAck (Ptr<Socket> socket, Address* dest, uint32_t id )
{
  std::string payload = std::string ("");
  Time expires_at = Time (Simulator::Now () + Hours (1));
  WildfireMessage message = WildfireMessage (id, WildfireMessageType::acknowledgement, &expires_at, &payload);
  SendMsg (socket, dest, &message);
}

void
WildfireClient::SendMsg (Ptr<Socket> socket, Address* dest, WildfireMessage* message)
{
  auto serialized_message = message->serialize ();
  Ptr<Packet> p = Create<Packet> (serialized_message->data (), serialized_message->size ());
  m_txTrace (p);
  socket->SendTo (p, 0, *dest);
}

void
WildfireClient::ScheduleSubscription (Time dt, Ipv4Address dest)
{
  NS_LOG_FUNCTION (this << dt);
  Simulator::Schedule (dt, &WildfireClient::SendSubscription, this, dest);
}

void
WildfireClient::SendSubscription (Ipv4Address dest)
{
  // Todo: move send details to seperate function
  Ptr<Packet> p;
  std::string message = std::string ("Subscription Request");
  Time expires_at = Time (Simulator::Now () + Hours (1));
  WildfireMessage alert = WildfireMessage (m_id, WildfireMessageType::subscribe, &expires_at, &message);
  m_id++;
  auto serialized_alert = alert.serialize ();
  p = Create<Packet> (serialized_alert->data (), serialized_alert->size ());
  Address localAddress;
  m_socket->GetSockName (localAddress);
  m_txTrace (p);
  m_txTraceWithAddresses (p, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (dest), 202));
  m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom (dest), 202));
  m_socket->Send (p);

  NS_LOG_INFO ("Wildfire Subscription Sent to " << dest);
}

} // Namespace ns3
