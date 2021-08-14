#include "wildfire-mobility-model.h"
#include "ns3/simulator.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE ("WildfireMobilityModel");
NS_OBJECT_ENSURE_REGISTERED (WildfireMobilityModel);

TypeId WildfireMobilityModel::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::WildfireMobilityModel")
    .SetParent<MobilityModel>()
    .SetGroupName ("Mobility")
    .AddConstructor<WildfireMobilityModel>()
  ;
  return tid;
}

TypeId WildfireMobilityModel::GetInstanceTypeId () const
{
  return WildfireMobilityModel::GetTypeId ();
}

WildfireMobilityModel::WildfireMobilityModel () {
  m_startTime = Time (0);
  m_velocity = Vector (0, 0, 0);
  m_startPosition = Vector (0, 0, 0);
  m_destination = Vector (0, 0, 0);
}

WildfireMobilityModel::~WildfireMobilityModel () {}

void WildfireMobilityModel::SetDestinationVelocity (const Vector &destination, const double &velocity)
{
  // Note Currently 2D only
  m_destination = destination;
  double slope = (m_destination.y - m_startPosition.y) /
    (m_destination.x - m_startPosition.x);
  m_theta = atan (slope);
  int16_t direction = m_destination.x > m_startPosition.x ? 1 : -1;
  NS_LOG_INFO ("Destination set with slope " << slope << " and theta " << m_theta);
  m_velocity = Vector ( sin (m_theta) * (velocity / 1000) * direction, cos (m_theta) * (velocity / 1000) * direction, 0);

  // Add 5 second delay
  m_startTime = (Simulator::Now () + Seconds (5));
}

inline Vector WildfireMobilityModel::DoGetVelocity (void) const
{
  NS_LOG_FUNCTION (this);
  return m_velocity;
}

inline Vector
WildfireMobilityModel::DoGetPosition (void) const
{
  // TODO: Stop at destination
  NS_LOG_FUNCTION (this);
  double elapsed_time = (Simulator::Now () - m_startTime).GetMilliSeconds ();
  if (elapsed_time < 0)
    {
      return m_startPosition;
    }

  return Vector (m_startPosition.x + m_velocity.x * elapsed_time,
                 m_startPosition.y + m_velocity.y * elapsed_time,
                 m_startPosition.x + m_velocity.z * elapsed_time );
}

void
WildfireMobilityModel::DoSetPosition (const Vector &position)
{
  NS_LOG_FUNCTION (this << position);
  m_velocity = DoGetVelocity ();
  m_startTime = Simulator::Now ();
  m_startPosition = position;
  NotifyCourseChange ();
}



}