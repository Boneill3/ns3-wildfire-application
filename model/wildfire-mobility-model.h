#ifndef WILDFIRE_MOBILITY_MODEL_H
#define WILDFIRE_MOBILITY_MODEL_H

#include "ns3/mobility-model.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{

class WildfireMobilityModel : public MobilityModel
{
public:
  /**
   * Register this type with the TypeId system.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);


  virtual TypeId GetInstanceTypeId () const;
  WildfireMobilityModel ();
  virtual ~WildfireMobilityModel ();
  /**
   * Set the model's destination and velocity
   */
  void SetDestinationVelocity (const Vector &destination, const double &velocity);

private:

  virtual Vector DoGetPosition (void) const;
  virtual void DoSetPosition (const Vector &position);
  virtual Vector DoGetVelocity (void) const;

  Time m_startTime;  //!< the start time
  Vector m_velocity; //!< the acceleration
  Vector m_startPosition; //!< the start position
  Vector m_destination; //!< the destination
  double m_theta; //!< Current Direction

  Time m_updatePeriod; //!< How often to update

};

}

#endif //WILDFIRE_MOBILITY_MODEL_H