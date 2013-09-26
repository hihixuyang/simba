// Please notice that the URDF file here is not strictlly the URDF format in the internet.
// Our version does has difference between the formal one.
// May consider support strict URDF format in the furture

#ifndef URDFPARSER_H
#define URDFPARSER_H

#include <URDFParser/TinXMLTool.h>
#include <ModelGraph/Body.h>
#include <ModelGraph/Model.h>
#include <Device/SimDeviceInfo.h>
#include <World/WorldManager.h>


// -----------------------------------------------------------------------------------------------------------------------
// parse world.xml file and build world in RobotProxy.
bool ParseWorld(const char* filename, WorldManager& mWroldManager)
{
    XMLDocument doc;
    if(doc.LoadFile(filename) !=0)
    {
         printf("Cannot open %s\n", filename);
         return false;
    }

    XMLElement *pParent=doc.RootElement();
    XMLElement *pElement=pParent->FirstChildElement();

    // read high level parent (root parent)
    while (pElement)
    {
        const char* sRootContent = pElement->Name();

        if(strcmp(sRootContent,"base")==0)
        {
            string  sMesh(pElement->Attribute("mesh"));
            mWroldManager.m_sMesh = sMesh;
            mWroldManager.iScale =::atoi( pElement->Attribute("scale"));
            mWroldManager.iMass =::atoi( pElement->Attribute("mass"));
            mWroldManager.vWorldPose = GenNumFromChar(pElement->Attribute("worldpose"));
            mWroldManager.vRobotPose= GenNumFromChar(pElement->Attribute("robotpose"));
            mWroldManager.vLightPose= GenNumFromChar(pElement->Attribute("lightpose"));
        }

        pElement=pElement->NextSiblingElement();
    }

    mWroldManager.PrintAll();

    return true;
}



// -----------------------------------------------------------------------------------------------------------------------
// parse robot.xml file and build robot model. Include robot body and all device body
// format of ‘robot name’ is robotname@proxyname. e.g. robot1@proxy1. All device will live under this name.
// This is also considered as RobotName.
// the name of any body of robot is: BodyName@RobotName@ProxyName, first, middle and last name
// the name of any joint of robot is: JointName@RobotName@ProxyName, first, middle and last name
bool ParseRobot(XMLDocument* doc, Model& m_RobotModel, Eigen::Vector6d& InitPose, string sProxyName)
{
    XMLElement *pParent=doc->RootElement();
    XMLElement *pElement=pParent->FirstChildElement();
    string sRobotName(GetAttribute(pParent,"name"));
    sRobotName = sRobotName+"@"+sProxyName;          // e.g. robot1@Proxy
    m_RobotModel.SetName(sRobotName);
    m_RobotModel.m_pParent = NULL;

    std::map<string, Body*>      m_mBodys;           // map to all body with key (name)

    // read high level parent (root parent)
    while (pElement)
    {
        const char* sRootContent = pElement->Name();
        // build body base (this is the body that most joint connect to)
        if(strcmp(sRootContent,"bodybase")==0)
        {
            string sBodyName = GetAttribute( pElement, "name")+"@"+sRobotName;
//            string sMeshdir = GetAttribute( pElement,"dir");
            int iMass =::atoi( pElement->Attribute("mass"));
            vector<double> vPose = GenNumFromChar(pElement->Attribute("pose"));
            InitPose<<vPose[0],vPose[1],vPose[2],vPose[3],vPose[4],vPose[5];
            vector<double> vDimesion= GenNumFromChar(pElement->Attribute("dimesion"));
            int iScale =::atoi( pElement->Attribute("scale"));

            const char* sType = pElement->Attribute("type");
            if(strcmp(sType, "Box") ==0)
            {
                BoxShape box = BoxShape(vDimesion[0],vDimesion[1],vDimesion[2]);
                Body* pBodyBase = new Body(sBodyName, box, iMass );
                pBodyBase->SetPose( vPose[0],vPose[1],vPose[2],vPose[3],vPose[4],vPose[5] );
                m_mBodys.insert(std::pair<std::string,Body*>(sBodyName,pBodyBase));
                m_RobotModel.SetBase( pBodyBase ); // main body
             }
        }

        // build body
        if(strcmp(sRootContent,"body")==0)
        {
            string sBodyName = GetAttribute( pElement, "name")+"@"+sRobotName;
            string sMeshdir(pElement->Attribute("dir"));
            int iMass =::atoi( pElement->Attribute("mass"));
            vector<double> vPose = GenNumFromChar(pElement->Attribute("pose"));
            vector<double> vDimesion = GenNumFromChar(pElement->Attribute("dimesion"));
            int iScale =::atoi( pElement->Attribute("scale"));

            const char* sType = pElement->Attribute("type");
            if(strcmp(sType, "Box") ==0)
            {
                BoxShape box = BoxShape(vDimesion[0],vDimesion[1],vDimesion[2]);
                Body* pBody = new Body(sBodyName, box, iMass );
                pBody->SetPose( vPose[0],vPose[1],vPose[2],vPose[3],vPose[4],vPose[5] );
                m_mBodys.insert(std::pair<std::string,Body*>(sBodyName,pBody));
            }
            else if(strcmp(sType,"Cylinder")==0)
            {
                CylinderShape cylinder = CylinderShape(vDimesion[0], vDimesion[1]);
                Body* pCylinder = new Body(sBodyName, cylinder, iMass);
                pCylinder->SetPose(vPose[0],vPose[1],vPose[2],vPose[3],vPose[4],vPose[5]);
                m_mBodys.insert(std::pair<std::string,Body*>(sBodyName,pCylinder));
            }
        }

        // create sim device
        if(strcmp(sRootContent,"Sensor")==0)
        {
            string sType( pElement->Attribute("Type"));

            if(sType == "Camera")
            {
                const char* sMode = pElement->Attribute("Mode");
                //----------------------------------------------------------------------------------------- Singel Camera
                if(strcmp(sMode, "RGB")==0 || strcmp(sMode,"Depth")==0 ||strcmp(sMode,"Gray")==0)
                {
                    string sCameraName= GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                    string sParentName= GetAttribute( pElement, "Parent")+"@"+sRobotName; // name of body that the sensor attach to. e.g. chassis@robot1@proxy
                    string sCamMode(sMode);
                    string sSensorName = sCamMode + sCameraName; // this is body name for sensor of the camera. e.g. RGBLCam@robot1@proxy
                    vector<double> vPose = GenNumFromChar(pElement->Attribute("Pose"));
                    int iMass = 1;
                    // string sMeshdir(pElement->Attribute("Dir"));

                    // create body for simcam
                    BoxShape box = BoxShape(0.1,0.1,0.1);
                    Body* pBody = new Body(sSensorName, box, iMass );
                    pBody->SetPose( vPose[0],vPose[1]+2,vPose[2]-3.2,vPose[3],vPose[4],vPose[5] );
                    m_mBodys.insert(std::pair<std::string,Body*>(sSensorName,pBody));

                    // create joint for SimCam
                    string sJointName = "SimCamJoint"+sCameraName; // e.g. SimCamJointLCam@robot1@proxy
                    Eigen::Vector3d vPivot;
                    Eigen::Vector3d vAxis;
                    vPivot<<vPose[0],vPose[1]+2,vPose[2]-3.2;
                    vAxis<<1,1,1;
                    HingeJoint* pHinge = new HingeJoint( sJointName, m_mBodys.find(sParentName)->second, m_mBodys.find(sSensorName)->second, vPivot[0], vPivot[1], vPivot[2], vAxis[0],vAxis[1],vAxis[2],1000,1000,0,3.1415926 );
                }

                // ---------------------------------------------------------------------------------------- RGB-Depth Camera
                if(strcmp(sMode, "RGBD")==0 )
                {
                    string sCameraName= GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                    string sParentName= GetAttribute( pElement, "Parent")+"@"+sRobotName; // name of body that the sensor attach to. e.g. chassis@robot1@proxy
                    vector<double> vPose = GenNumFromChar(pElement->Attribute("Pose"));
                    int iMass = 1;
                    double BodyDistance = 0.5;
                    // string sMeshdir(pElement->Attribute("Dir"));

                    /// create bodys
                    // 1.3 create body to connect RGB Cam and Depth Cam
                    BoxShape box = BoxShape(BodyDistance,0.1,0.1);
                    Body* pBody = new Body(sCameraName, box, iMass );
                    pBody->SetPose(0,-2.1,0, 0, 0, 0 );
                    m_mBodys.insert(std::pair<std::string,Body*>(sCameraName,pBody));


                    /// create joints
                    Eigen::Vector3d vPivot;
                    Eigen::Vector3d vAxis;
                    // 2.3 create joint for SimCam and parent

                    string sJointName = "SimCamJoint"+sCameraName;
                    vPivot<< 0, 0, 0;
                    vAxis<<0,-1,0;
                    HingeJoint* pRGBDHinge = new HingeJoint( sJointName,
                                                             m_mBodys.find(sParentName)->second,
                                                             pBody,
                                                             vPivot[0], vPivot[1], vPivot[2],
                                                             vAxis[0],vAxis[1],vAxis[2],
                                                             100,100,0,0.01 );

                    // 1.1 create body for RGB Cam
                    string sRGBBodyName = "RGB"+sCameraName;
                    BoxShape RGBbox = BoxShape(0.1,0.1,0.1);
                    Body* pRGBBody = new Body(sRGBBodyName, RGBbox, iMass );
                    pRGBBody->SetPose( vPose[0]-BodyDistance - 0.1,vPose[1] + 2, vPose[2], 0, 0, 0 );
                    m_mBodys.insert(std::pair<std::string,Body*>(sRGBBodyName,pRGBBody));

                    // 2.1 create joint for RGB body and RGBDCamBody
                    string sRGBJointName = "SimCamJoint"+sRGBBodyName;
                    vPivot<<-0.5, 0, 0;
                    vAxis<< -1,0,0;
        //                    HingeJoint* pRGBHinge = new HingeJoint( sRGBJointName, m_mBodys.find(sCameraName)->second, m_mBodys.find(sRGBBodyName)->second, vPivot[0], vPivot[1], vPivot[2], vAxis[0],vAxis[1],vAxis[2], 100, 100, -M_PI, M_PI );
                    HingeJoint* pRGBHinge = new HingeJoint( sRGBJointName, pBody, pRGBBody, vPivot[0], vPivot[1], vPivot[2], vAxis[0],vAxis[1],vAxis[2], 100, 100, -M_PI, M_PI );

                    // 1.2 create body for Depth Cam
                    string sDepthBodyName = "Depth"+sCameraName;
                    BoxShape Depthbox = BoxShape(0.1,0.1,0.1);
                    Body* pDepthBody = new Body(sDepthBodyName, Depthbox, iMass );
                    pDepthBody->SetPose( vPose[0]+BodyDistance+0.1,vPose[1] + 2,vPose[2], 0, 0, 0 );
                    m_mBodys.insert(std::pair<std::string,Body*>(sDepthBodyName,pDepthBody));

                    // 2.2 create joint for Depth body and RGBDCamBody
                    string sDepthJointName = "SimCamJoint"+sDepthBodyName;
                    vPivot<<0.5, 0, 0;
                    vAxis<<  1, 0, 0;
                    HingeJoint* pDepthHinge = new HingeJoint( sDepthJointName,
                                                              pBody,
                                                              pDepthBody,
                                                              vPivot[0], vPivot[1], vPivot[2],
                                                              vAxis[0],vAxis[1],vAxis[2],
                                                              1, 0, 0,
                                                              100,100,-M_PI,M_PI );
                }
            }
            // ---------------------------------------------------------------------------------------- RGB-Depth Camera
            if(sType=="GPS")
            {
                string sBodyName = GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                string sParentName= GetAttribute( pElement, "Parent")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                vector<double> vPose = GenNumFromChar(pElement->Attribute("Pose"));
                int iMass = 1;
                // string sMeshdir(pElement->Attribute("Dir"));

                // create body for it
                BoxShape box = BoxShape(0.1,0.1,0.1);
                Body* pBody = new Body(sBodyName, box, iMass );
                pBody->SetPose( vPose[0],vPose[1],vPose[2],vPose[3],vPose[4],vPose[5] );
                m_mBodys.insert(std::pair<std::string,Body*>(sBodyName,pBody));

                // create joint for SimGPS
                string sJointName = "GPSJoint"+sBodyName;
                Eigen::Vector3d vPivot;
                Eigen::Vector3d vAxis;
                vPivot<<vPose[0],vPose[1],vPose[2];
                vAxis<<1,0,0;
                HingeJoint* pHinge = new HingeJoint( sJointName, m_mBodys.find(sParentName)->second, m_mBodys.find(sBodyName)->second, vPivot[0], vPivot[1], vPivot[2], vAxis[0],vAxis[1],vAxis[2],100,100,0,M_PI );
            }

        }

        // construct parent, (joint)
        if(strcmp(sRootContent,"joint")==0)
        {
            string sJointName= GetAttribute( pElement, "name")+"@"+sRobotName; // get joint name. e.g. BLAxleJoint@robot1@proxy
            string sJointType(pElement->Attribute("type"));

            if(sJointType == "HingeJoint")
            {
                string sParentName;
                string sChildName;
                vector<double> vPivot;
                vector<double> vAxis;
                double dUpperLimit = M_PI;
                double dLowerLimit = 0;
                double dDamping = 1;
                double dStiffness = 1;

                // Construct joint based on children information. This information may include links, origin, axis.etc
                XMLElement *pChild=pElement->FirstChildElement();                
                while(pChild)
                {
                    const char * sName = pChild->Name();
                    // get parent body of joint
                    if(strcmp(sName, "parent")==0)
                    {
                        sParentName = GetAttribute(pChild, "body") +"@"+sRobotName;
                    }

                    // get child body of joint
                    if(strcmp(sName, "child")==0)
                    {
                        sChildName = GetAttribute(pChild, "body") +"@"+sRobotName;
                    }
                    if(strcmp(sName, "pivot")==0)
                    {
                        vPivot=GenNumFromChar( pChild->Attribute("setting"));
                    }
                    if(strcmp(sName, "axis")==0)
                    {
                        vAxis=GenNumFromChar( pChild->Attribute("setting"));
                    }
                    if(strcmp(sName,"upperlimit")==0)
                    {
                        dUpperLimit = ::atof(sName);
                    }
                    if(strcmp(sName,"lowerlimit")==0)
                    {
                        dLowerLimit = ::atof(sName);
                    }
                    if(strcmp(sName,"damping")==0)
                    {
                        dDamping = ::atof(sName);
                    }
                    if(strcmp(sName,"stiffness")==0)
                    {
                        dStiffness = ::atof(sName);
                    }

                    // read next child (joint)
                    pChild=pChild->NextSiblingElement();
                }

                HingeJoint* pHinge = new HingeJoint( sJointName, m_mBodys.find(sParentName)->second, m_mBodys.find(sChildName)->second,
                                                     vPivot[0], vPivot[1], vPivot[2], vAxis[0],vAxis[1],vAxis[2],dStiffness,dDamping,dLowerLimit,dUpperLimit );

            }
            else if(sJointType=="Hinge2Joint")
            {
                string sParentName;
                string sChildName;
                vector<double> vAnchor;
                vector<double> vAxis1;
                vector<double> vAxis2;
                vector<double> vLowerLinearLimit;
                vector<double> vUpperLinearLimit;
                vector<double> vLowerAngleLimit;
                vector<double> vUpperAngleLimit;

                Eigen::Vector3d Anchor;
                Eigen::Vector3d Axis1;
                Eigen::Vector3d Axis2;
                Eigen::Vector3d LowerLinearLimit;
                Eigen::Vector3d UpperLinearLimit;
                Eigen::Vector3d LowerAngleLimit;
                Eigen::Vector3d UpperAngleLimit;

                // read detail of a joint
                XMLElement *pChild=pElement->FirstChildElement();

                // Construct joint based on children information. This information may include links, origin, axis.etc
                while(pChild)
                {
                    const char * sName = pChild->Name();

                    // get parent link of joint
                    if(strcmp(sName, "parent")==0)
                    {
                        sParentName = GetAttribute(pChild, "body") +"@"+sRobotName;
                    }
                    // get child link of joint
                    if(strcmp(sName, "child")==0)
                    {
                        sChildName = GetAttribute(pChild, "body") +"@"+sRobotName;
                    }
                    if(strcmp(sName, "anchor")==0)
                    {
                        vAnchor = GenNumFromChar( pChild->Attribute("setting"));
                        Anchor<<vAnchor[0],vAnchor[1],vAnchor[2];
                    }
                    if(strcmp(sName, "axis")==0)
                    {
                        vAxis1 = GenNumFromChar( pChild->Attribute("axis1"));
                        vAxis2 = GenNumFromChar( pChild->Attribute("axis2"));

                        Axis1<<vAxis1[0],vAxis1[1],vAxis1[2];
                        Axis2<<vAxis2[0],vAxis2[1],vAxis2[2];
                    }
                    if(strcmp(sName, "limit")==0)
                    {
                        vLowerLinearLimit = GenNumFromChar( pChild->Attribute("lowerlinear"));
                        vUpperLinearLimit = GenNumFromChar( pChild->Attribute("upperlinear"));
                        vLowerAngleLimit = GenNumFromChar( pChild->Attribute("lowerangle"));
                        vUpperAngleLimit = GenNumFromChar( pChild->Attribute("upperangle"));

                        LowerAngleLimit<<vLowerAngleLimit[0],vLowerAngleLimit[1],vLowerAngleLimit[2];
                        UpperAngleLimit<<vUpperAngleLimit[0],vUpperAngleLimit[1],vUpperAngleLimit[2];
                        LowerLinearLimit<<vLowerLinearLimit[0],vLowerLinearLimit[1],vLowerLinearLimit[2];
                        UpperLinearLimit<<vUpperLinearLimit[0],vUpperLinearLimit[1],vUpperLinearLimit[2];
                    }
                    // read next child (joint)
                    pChild=pChild->NextSiblingElement();
                }

                Hinge2Joint* pHinge2 = new Hinge2Joint( sJointName, m_mBodys.find(sParentName)->second, m_mBodys.find(sChildName)->second,
                                                        Axis1, Axis2, Anchor, 0.2, 0.2, LowerLinearLimit, UpperLinearLimit, LowerAngleLimit, UpperAngleLimit);

                std::cout<<"Creating a Hinge2Joint between "<<sParentName<<" and "<<sChildName<<std::endl;
            }
        }

        // read next parent element
        pElement=pElement->NextSiblingElement();
    }

    return true;
}



// -----------------------------------------------------------------------------------------------------------------------
// parse Robot urdf file to extract all devices and build vSimDeviceInfo
// format of robot name is robotname@proxyname. e.g. robot1@proxy1. All device will live under this name
bool ParseDevice(XMLDocument& doc, vector<SimDeviceInfo>&  m_vSimDeviceInfo, string sProxyName)
{
    XMLElement *pParent=doc.RootElement();
    XMLElement *pElement=pParent->FirstChildElement();
    string sRobotName(GetAttribute(pParent,"name"));
    sRobotName = sRobotName+"@"+sProxyName;          // e.g. robo

    // read high level parent (root parent)
    while (pElement)
    {
        const char* sRootContent = pElement->Name();

        // create sim Sensor device
        if(strcmp(sRootContent,"Sensor")==0)
        {
            string sType( pElement->Attribute("Type"));

            if(sType == "Camera")
            {
                const char* sMode = pElement->Attribute("Mode");
                string sModel = GetAttribute(pElement, "Model");

                //----------------------------------------------------------------------------------------- Singel Camera
                if(strcmp(sMode, "RGB")==0 || strcmp(sMode,"Depth")==0 ||strcmp(sMode,"Gray")==0)
                {
                    string sCameraName= GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                    string sCamMode(sMode);
                    string sSensorName = sCamMode + sCameraName; // this is body name for sensor of the camera. e.g. RGBLCam@robot1@proxy
                    int iFPS=atoi( GetAttribute(pElement,"FPS").c_str());

                    // save device info
                    SimDeviceInfo Device;
                    Device.sDeviceName = sCameraName;
                    Device.sDeviceType = sType;
                    Device.m_iFPS = iFPS;
                    Device.m_vSensorList.push_back(sSensorName);
                    Device.m_vModel.push_back(sModel);
                    m_vSimDeviceInfo.push_back(Device);

                    cout<<"[Proxy/ParseDevice] register "<<sType<<" (SimCam "<<sSensorName<<") success. Device Name is "<<sCameraName<<"."<<endl;
                }

                // ---------------------------------------------------------------------------------------- RGB-Depth Camera
                if(strcmp(sMode, "RGBD")==0 )
                {
                    string sCameraName= GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                    string sRGBBodyName = "RGB"+sCameraName;
                    string sDepthBodyName = "Depth"+sCameraName;
                    int iFPS=atoi( GetAttribute(pElement,"FPS").c_str());

                    // 3 save intp device, this device have two sensors
                    SimDeviceInfo Device;
                    Device.sDeviceName = sCameraName;
                    Device.sDeviceType = sType;
                    Device.m_iFPS = iFPS;
                    Device.m_vSensorList.push_back(sRGBBodyName);
                    Device.m_vSensorList.push_back(sDepthBodyName);
                    Device.m_vModel.push_back(sModel);
                    Device.m_vModel.push_back(sModel);
                    m_vSimDeviceInfo.push_back(Device);

                    cout<<"[Proxy/ParseDevice] register "<<sType<<" (SimCam "<<sMode<<") success." <<endl;
                }
            }

            if(sType=="GPS")
            {
                string sBodyName = GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy

                cout<<"[Proxy/ParseDevice] Register Sim GPS success."<<endl;
            }


            if(sType=="Vicon")
            {
                string sViconName = GetAttribute( pElement, "Name")+"@"+sRobotName;// name of the camera. e.g. LCam@robot1@proxy
                string sBodyName= GetAttribute( pElement, "Body")+"@"+sRobotName;

                SimDeviceInfo Device;
                Device.sDeviceName = sViconName;
                Device.sDeviceType = sType;
                Device.sBodyName = sBodyName;
                m_vSimDeviceInfo.push_back(Device);
                cout<<"[Proxy/ParseDevice] Add vicon device "<<sViconName<<" success."<<endl;
            }
        }

        // create sim controller device
        if(strcmp(sRootContent, "Controller")==0)
        {
            string sType( sRootContent );
            string sMode( pElement->Attribute("Mode"));

            if(sMode == "SimpleController")
            {
                string  sControllerName = GetAttribute(pElement, "Name");

                SimDeviceInfo Device;
                Device.sDeviceName = sControllerName;
                Device.sDeviceType = sType;
                Device.sDeviceMode = sMode;
                Device.sRobotName = sRobotName;
                m_vSimDeviceInfo.push_back(Device);
                cout<<"[Proxy/ParseDevice] Add controller device "<<sControllerName<<" success."<<endl;
            }

            if(sType =="CarController")
            {


            }
        }

        // read next parent element
        pElement=pElement->NextSiblingElement();
    }

    return true;
}



#endif
