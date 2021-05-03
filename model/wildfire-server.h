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
#ifndef WILDFIRE_SERVER_H
#define WILDFIRE_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include <vector>

namespace ns3 {

class Socket;
class Packet;

class WildfireServer : public Application
{
  public:
    WildfireServer();
    virtual ~WildfireServer();
    static TypeId GetTypeId(void);

  protected:
    virtual void DoDispose (void);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    /**
     * \brief Handle a packet reception.
     *
     * This function is called by lower layers.
     *
     * \param socket the socket the packet was received to.
     */
    void HandleRead (Ptr<Socket> socket);

    uint16_t m_port; //!< Port on which we listen for incoming packets.
    Ptr<Socket> m_socket; //!< IPv4 Socket

    /// Callbacks for tracing the packet Rx events
    TracedCallback<Ptr<const Packet> > m_rxTrace;

    /// Callbacks for tracing the packet Rx events, includes source and destination addresses
    TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses; 

    std::vector<Address> subscribers;
};

} // namespace ns3
#endif /* WILDFIRE_SERVER_H */