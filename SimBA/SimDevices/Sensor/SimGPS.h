#ifndef SIMGPS_H
#define SIMGPS_H

#include <SimDevices/SimDevices.h>
#include <ModelGraph/PhysicsEngine.h>.h>

// return x,y,z of a GPS sensor.
class SimGPS
{
public:
    void init(string DeviceName, PhysicsEngine* mPhysWrapper)
    {
        m_pPhysWrapper = mPhysWrapper;
        m_sDeviceName = DeviceName;
    }

    void Update()
    {
        m_CurPose = m_pPhysWrapper->GetEntityOrigin(m_sDeviceName);
    }

    void GetPose(Eigen::Vector3d &pose)
    {
        pose<<m_CurPose[0], m_CurPose[1],m_CurPose[2];
    }

    void PrintPose()
    {
        cout<<"[GPS PrintPose] current pose is x:"<<m_CurPose[0]<<" y:"<<m_CurPose[1]<<" z:"<<m_CurPose[2]<<endl;
    }

private:
    PhysicsEngine*               m_pPhysWrapper;
    string                        m_sDeviceName;
    Eigen::Vector3d               m_CurPose;
};

#endif // SIMGPS_H