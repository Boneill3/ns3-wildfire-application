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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <vector>

#include "wildfire-server.h"

NS_LOG_COMPONENT_DEFINE ("WildfireServerApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WildfireServer);

WildfireServer::WildfireServer ()
{
  NS_LOG_FUNCTION (this);
  m_privateKey = new std::string ("PRIVATEKEY");
  m_publicKey = new std::string ("PUBLICKEY");
}

WildfireServer::~WildfireServer ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  delete m_privateKey;
  delete m_publicKey;
}

TypeId
WildfireServer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::WildfireServer")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<WildfireServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&WildfireServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&WildfireServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&WildfireServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("Tx", "A packet has been sent",
                     MakeTraceSourceAccessor (&WildfireServer::m_txTrace),
                     "")
    .AddTraceSource ("Ack", "An Ack packet has been received",
                     MakeTraceSourceAccessor (&WildfireServer::m_ackTrace),
                     "")
    .AddTraceSource ("Sub", "A Subscription packet has been received",
                     MakeTraceSourceAccessor (&WildfireServer::m_subTrace),
                     "")
  ;
  return tid;
}

void
WildfireServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
WildfireServer::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&WildfireServer::HandleRead, this));
}

void
WildfireServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
WildfireServer::ScheduleNotification (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &WildfireServer::SendNotification, this);
}

void
WildfireServer::SendNotification ()
{
  Ptr<Packet> p;
  std::string message = std::string ("Level 2 Alert");
  Time expires_at = Simulator::Now () + Seconds (30);
  WildfireMessage alert = WildfireMessage (id, WildfireMessageType::notification, &expires_at, &message);
  id++;
  for (auto subscriber = subscribers.begin (); subscriber != subscribers.end (); ++subscriber)
    {
      //NS_ASSERT (m_socket->Connect (InetSocketAddress::ConvertFrom (*subscriber)) != -1);
      Address address = std::get<0> (*subscriber);
      Ptr<Socket> socket = std::get<1> (*subscriber);
      SendMsg (socket, &(address), &alert);
      NS_LOG_INFO ("Wildfire Notification SENT to " <<
                   InetSocketAddress::ConvertFrom (address).GetIpv4 () << " port " <<
                   InetSocketAddress::ConvertFrom (address).GetPort ());
    }
}

bool
WildfireServer::HandleRequest (Ptr<Socket> socket, const Address & source)
{
  NS_LOG_INFO ("HandleRequest");
  return true;
}

void
WildfireServer::HandleAccept (Ptr<Socket> socket, const Address & source)
{
  NS_LOG_INFO ("HandleAccept");
  socket->SetRecvCallback (MakeCallback (&WildfireServer::HandleRead, this));
}

void
WildfireServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_INFO ("HandleRead");

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }

      // Get data from packet
      uint32_t size = packet->GetSize ();
      uint8_t buffer[size];
      packet->CopyData (buffer, size);

      // convert data to application message
      std::vector<uint8_t> vbuffer (buffer, buffer + sizeof buffer / sizeof buffer[0]);
      WildfireMessage message = WildfireMessage (&vbuffer);

      //NS_LOG_INFO(buffer);
      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      if(message.getType () == WildfireMessageType::subscribe)
        {
          NS_LOG_INFO ("Adding Subscriber");
          subscribers.push_back (std::make_tuple (from, socket));

          m_subTrace ();

          // Send Success Ack
          Ptr<Packet> ack;
          Time expires_at = Simulator::Now () + Seconds (30);
          WildfireMessage ack_message = WildfireMessage (message.getId (), WildfireMessageType::acknowledgement, &expires_at, m_publicKey);
          SendMsg (socket, &from, &ack_message);
        }

      else if (message.getType () == WildfireMessageType::acknowledgement)
        {
          m_ackTrace ();
          NS_LOG_INFO ("Ack Received on Server");
        }

    }
}

void
WildfireServer::SendMsg (Ptr<Socket> socket, Address* dest, WildfireMessage* message)
{
  auto serialized_message = message->serialize ();
  Ptr<Packet> p = Create<Packet> (serialized_message->data (), serialized_message->size ());
  m_txTrace ();
  socket->SendTo (p, 0, *dest);
}

} // Namespace ns3