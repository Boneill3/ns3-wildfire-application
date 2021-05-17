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
// Message Format
// 0   - 31  Id (int32)
// 32  - 33  type (u8)
// 33  - 122 Message 90 char (string)
// 123 - 155 hash (128 bits/ 32 characters)
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

  if(message.size () != 4)
    {
      m_message = new std::string (data->begin (), data->end ());
      m_type = 0;
      m_id = 0;
      m_hash = new std::string ("");
      return;
    }

  m_id = static_cast<uint32_t> (std::stoul (message[0]));
  m_type = static_cast<uint8_t> (std::stoul (message[1]));
  m_message = new std::string (message[2]);
  m_hash = new std::string (message[3]);
}

WildfireMessage::WildfireMessage (uint32_t id, uint8_t type, std::string* message)
{
  m_id = id;
  m_type = type;
  m_message = new std::string (*message);
  m_hash = new std::string ("12345678901234567890123456789012");
}

WildfireMessage::~WildfireMessage ()
{
  delete m_message;
  delete m_hash;
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

std::vector<uint8_t>* WildfireMessage::serialize ()
{
  std::string myString = std::to_string (m_id) + '|' + std::to_string (m_type) + '|' + *m_message + '|' + *m_hash;
  return new std::vector<uint8_t> (myString.begin (), myString.end ());
}

bool WildfireMessage::isValid (std::string* key)
{
  return true;
}

std::string* WildfireMessage::toString ()
{
  return new std::string (std::to_string (m_id) + "," + std::to_string (m_type) + "," + *m_message + "," + *m_hash);
}

}