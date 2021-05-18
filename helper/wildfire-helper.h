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
#ifndef WILDFIRE_HELPER_H
#define WILDFIRE_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"

namespace ns3 {

class WildfireServerHelper
{
  public:
    WildfireServerHelper(uint16_t port);
    void SetAttribute (std::string name, const AttributeValue &value);
    ApplicationContainer Install (Ptr<Node> node) const;
    ApplicationContainer Install (std::string nodeName) const;
    ApplicationContainer Install (NodeContainer c) const;
    void ScheduleNotification(Ptr<Application> app, Time dt);
  
  private:
    Ptr<Application> InstallPriv (Ptr<Node> node) const;
    ObjectFactory m_factory;  
};

class WildfireClientHelper
{
public:

  WildfireClientHelper (Address ip, uint16_t port);
  WildfireClientHelper (Address addr);
  void SetAttribute (std::string name, const AttributeValue &value);
  void SetFill (Ptr<Application> app, std::string fill);
  void SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength);
  void SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength);
  ApplicationContainer Install (Ptr<Node> node) const;
  ApplicationContainer Install (std::string nodeName) const;
  ApplicationContainer Install (NodeContainer c) const;
  void ScheduleSubscription(Ptr<Application> app, Time dt, Ipv4Address dest);

private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
  ObjectFactory m_factory; //!< Object factory.
};

}
#endif /* WILDFIRE_HELPER_H */