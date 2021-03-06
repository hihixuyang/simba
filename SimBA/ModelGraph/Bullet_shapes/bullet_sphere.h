#ifndef BULLET_SPHERE_H
#define BULLET_SPHERE_H

#include <bullet/BulletCollision/CollisionShapes/btSphereShape.h>
#include <bullet/LinearMath/btAlignedAllocator.h>
#include "bullet_shape.h"

class bullet_sphere : public bullet_shape{

public:
  //constructor
  bullet_sphere(ModelNode* mnSphere){
    SphereShape* pSphere = (SphereShape*) mnSphere;
    double dRadius = pSphere->m_dRadius;
    double dMass = pSphere->GetMass();
    double dRestitution = pSphere->GetRestitution();
    Eigen::Matrix4d dPose;
    dPose = pSphere->GetPoseMatrix();

    bulletShape = new btSphereShape(dRadius);
    bulletMotionState = new NodeMotionState(*mnSphere);
    bool isDynamic = ( dMass != 0.f );
    btVector3 localInertia( 0, 0, 0 );
    if( isDynamic ){
        bulletShape->calculateLocalInertia( dMass, localInertia );
    }
    btRigidBody::btRigidBodyConstructionInfo  cInfo(dMass, bulletMotionState,
                                                    bulletShape, localInertia);
    bulletBody = new btRigidBody(cInfo);
    double dContactProcessingThreshold = 0.001;
    bulletBody->setContactProcessingThreshold( dContactProcessingThreshold );
    bulletBody->setRestitution( dRestitution );
    SetPose(dPose);
  }

};


#endif // BULLET_SPHERE_H
