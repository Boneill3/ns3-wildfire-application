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

#ifndef WILDFIRE_MESSAGE_H
#define WILDFIRE_MESSAGE_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"

namespace ns3
{

enum WildfireMessageType { error, subscribe, unsubscribe, notification, acknowledgement };
class WildfireMessage
{
private:
  std::string* m_message;
  std::string* m_hash;
  std::uint8_t m_type;
  uint32_t m_id;

public:
  WildfireMessage (std::vector<uint8_t>* data);
  WildfireMessage (uint32_t id, uint8_t type, std::string* message);
  ~WildfireMessage ();
  uint32_t getId ();
  std::string* getMessage ();
  WildfireMessageType getType ();
  std::string* getHash ();
  std::vector<uint8_t>* serialize ();
  bool isValid (std::string* key);
  std::string* toString ();
};


} // namespace ns3


#endif /* WILDFIRE_MESSAGE_H */