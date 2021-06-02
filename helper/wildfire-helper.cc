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
#include "wildfire-helper.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

#include "ns3/wildfire-server.h"
#include "ns3/wildfire-client.h"

namespace ns3 {

WildfireServerHelper::WildfireServerHelper (uint16_t port)
{
  m_factory.SetTypeId (WildfireServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
WildfireServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value
  )
{
  m_factory.Set (name, value);
}

ApplicationContainer
WildfireServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
WildfireServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
WildfireServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
WildfireServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<WildfireServer> ();
  node->AddApplication (app);

  return app;
}

void
WildfireServerHelper::ScheduleNotification (Ptr<Application> app, Time dt)
{
  app->GetObject<WildfireServer>()->ScheduleNotification (dt);
}

WildfireClientHelper::WildfireClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (WildfireClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

WildfireClientHelper::WildfireClientHelper (Address address)
{
  m_factory.SetTypeId (WildfireClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
WildfireClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
WildfireClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
WildfireClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
WildfireClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
WildfireClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<WildfireClient> ();
  node->AddApplication (app);

  return app;
}

void
WildfireClientHelper::ScheduleSubscription (Ptr<Application> app, Time dt, Ipv4Address dest)
{
  app->GetObject<WildfireClient>()->ScheduleSubscription (dt, dest);
}

} // namespace ns3