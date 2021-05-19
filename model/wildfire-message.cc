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

#include "wildfire-message.h"
namespace ns3
{

WildfireMessage::WildfireMessage (std::vector<uint8_t>* data)
{
  std::string s (data->begin (), data->end ());
  size_t pos = 0;
  std::string token;
  std::vector<std::string> message;
  while ((pos = s.find ('|')) != std::string::npos)
    {
      token = s.substr (0, pos);
      message.push_back (token);
      s.erase (0, pos + 1);
    }
  message.push_back (s);

  if(message.size () != 5)
    {
      m_message = new std::string (data->begin (), data->end ());
      m_type = 0;
      m_id = 0;
      m_expires_at = new Time (Simulator::Now ());
      m_hash = new std::string ("");
      return;
    }

  m_id = static_cast<uint32_t> (std::stoul (message[0]));
  m_type = static_cast<uint8_t> (std::stoul (message[1]));
  m_expires_at = new Time (message[2]);
  m_message = new std::string (message[3]);
  m_hash = new std::string (message[4]);
}

WildfireMessage::WildfireMessage (uint32_t id, uint8_t type, Time* expires_at, std::string* message)
{
  m_id = id;
  m_type = type;
  m_expires_at = new Time (*expires_at);
  m_message = new std::string (*message);
  m_hash = new std::string ("12345678901234567890123456789012");
}

WildfireMessage::~WildfireMessage ()
{
  delete m_message;
  delete m_hash;
  delete m_expires_at;
}

uint32_t WildfireMessage::getId ()
{
  return m_id;
}

std::string* WildfireMessage::getMessage ()
{
  return new std::string (*m_message);
}

std::string* WildfireMessage::getHash ()
{
  return new std::string (*m_hash);
}

WildfireMessageType WildfireMessage::getType ()
{
  return static_cast<WildfireMessageType> (m_type);
}

std::vector<uint8_t>* WildfireMessage::serialize ()
{
  std::string myString = std::to_string (m_id) + '|' + std::to_string (m_type) + '|' + std::to_string (m_expires_at->ToDouble (Time::Unit::S)) + '|' +
    *m_message + '|' + *m_hash;
  return new std::vector<uint8_t> (myString.begin (), myString.end ());
}

bool WildfireMessage::isValid (std::string* key)
{
  return true;
}

bool WildfireMessage::isExpired ()
{
  return *m_expires_at < Simulator::Now ();
}

std::string* WildfireMessage::toString ()
{
  return new std::string (std::to_string (m_id) + "," + std::to_string (m_type) + "," + std::to_string (m_expires_at->ToDouble (Time::Unit::S)) +
                          "," + *m_message + "," + *m_hash);
}

}