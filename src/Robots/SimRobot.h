#ifndef SIMROBOT_H
#define SIMROBOT_H

//#include <Network/Messages.h>
#include <SceneGraph/SceneGraph.h> // for open GL scene graph
#include <ModelGraph/Models.h>
#include <ModelGraph/RenderClass.h>
#include <ModelGraph/PhysicsClass.h>
#include <ModelGraph/ModelGraphBuilder.h>
#include <ModelGraph/PhyModelGraphAgent.h>
#include <URDFParser/RobotProxyURDFParser.h>
#include <Device/Controller/Controller.h>
#include <Utils/ConvertName.h>


// Sim Robot (can works with PID control and bicycle & uni-cycle models).
// This can be any kind of robot, a Car, Pan-Tilt...
// This can be main robot (my robot) or other player's robot.
class SimRobot
{
public:
       // init mesh (appearance) of our robot. The initial pose of the robot is decided in the following two ways
       // 1, if proxy runs in non-StateKeeper mode, the initial pose of robot is readed from its's urdf file.
       // 2, if proxy runs in on-StateKeeper mode, the initial pose of robot is readed from StateKeeper.
       ~SimRobot()
       {
           // 1, delete all constraints
           // 1.1, delete all hinge constriants
           std::map<string, btHingeConstraint*>::iterator Hiter =m_rPhyMGAgent.m_Agent.GetPhys()->m_mHingeJointList.begin();
           for(; Hiter!=m_rPhyMGAgent.m_Agent.GetPhys()->m_mHingeJointList.end(); Hiter++)
           {
               string sFullHingeConstraintName = Hiter->first;
               int index = sFullHingeConstraintName.find("@");
               if(index!=-1)
               {
                   string substr = sFullHingeConstraintName.substr(index+1, sFullHingeConstraintName.size()-index);

                   if(substr == m_sRobotName)
                   {
                       cout<<"delete "<<sFullHingeConstraintName<<endl;
                       m_rPhyMGAgent.m_Agent.GetPhys()->DeleteHingeConstraintFromDynamicsWorld(sFullHingeConstraintName);
                       m_rPhyMGAgent.m_Agent.GetPhys()->m_mHingeJointList.erase(Hiter);
                   }
               }
           }


           // 1.2, delete all hinge2 constraints
           std::map<string, btHinge2Constraint*>::iterator H2iter =m_rPhyMGAgent.m_Agent.GetPhys()->m_mHinge2JointList.begin();
           for(; H2iter!=m_rPhyMGAgent.m_Agent.GetPhys()->m_mHinge2JointList.end(); H2iter++)
           {
               string sFullHinge2ConstraintName = H2iter->first;
               int index = sFullHinge2ConstraintName.find("@");
               if(index!=-1)
               {
                   string substr = sFullHinge2ConstraintName.substr(index+1, sFullHinge2ConstraintName.size()-index);

                   if(substr == m_sRobotName)
                   {
                       cout<<"delete "<<sFullHinge2ConstraintName<<endl;
                       m_rPhyMGAgent.m_Agent.GetPhys()->DeleteHinge2ConstraintFromDynamicsWorld(sFullHinge2ConstraintName);
                       m_rPhyMGAgent.m_Agent.GetPhys()->m_mHinge2JointList.erase(H2iter);
                   }
               }
           }


           // 2, delete all body and entity
           std::map<string, boost::shared_ptr<Entity> >::iterator iter =m_rPhyMGAgent.m_Agent.GetPhys()->m_mEntities.begin();
           for(;iter!=m_rPhyMGAgent.m_Agent.GetPhys()->m_mEntities.end(); iter++)
           {
               string sFullName = iter->first;
               int index = sFullName.find("@");
               if(index!=-1)
               {
                   string substr = sFullName.substr(index+1, sFullName.size()-index);

                   if(substr == m_sRobotName)
                   {
                       cout<<"try to delete "<<sFullName<<endl;
                       m_rPhyMGAgent.m_Agent.GetPhys()->DeleteRigidbodyFromDynamicsWorld(sFullName);
//                     m_rPhyMGAgent.m_Agent.GetPhys()->m_mEntities.erase(iter); //**** need to delete it. implement it in the furture ****
                   }
               }
           }
       }



       // -----------------------------------------------------------------------------------------------------------------
       bool Init(string ProxyName,bool bStateKeeperOn ,PhyModelGraphAgent& rPhyMGAgent, Render& rRender, XMLDocument& doc)
       {
           m_sProxyName = ProxyName;
           m_pRobotURDF = doc.ToDocument();

           if( ParseRobot(m_pRobotURDF->GetDocument(), m_RobotModel, m_eRobotInitPoseInURDF, m_sProxyName)==false )
           {
               return false;
           }

           m_sRobotName = m_RobotModel.GetName();

           m_rPhyMGAgent = rPhyMGAgent;
           m_Render = rRender;

           // If we run in 'without Network mode', we need to init robot pose ourselves. or this pose will be read from server.
           // and we need to call AddRobotInModelGraph() manaully once we get the pose from statekeeper
           if(bStateKeeperOn == false)
           {
               Eigen::Vector6d RobotInitPoseInWorld;
               RobotInitPoseInWorld<<0,0,-2,0,0,1.57;
               InitPoseOfBodyBaseWRTWorld(RobotInitPoseInWorld);
               AddRobotInModelGraph();
           }

           cout<<"[SimRobot] Build robot "<<m_sRobotName<<" success."<<endl;

           return true;
       }



       // -----------------------------------------------------------------------------------------------------------------
       // add robot in Physic & Render. The last step of building a robot.
       // user can choice to build physic or render here
       void AddRobotInModelGraph()
       {
           m_MGBuilder.Init(ModelGraphBuilder::BuildPhysic, m_rPhyMGAgent.m_Agent.GetPhys(), m_Render,m_RobotModel, m_eRobotInitPose);
           cout<<"add model to model graph success."<<endl;
       }



       // -----------------------------------------------------------------------------------------------------------------
       // here eRobotInitPoseInWorld is the pose of badybase of a robot with respect to world.
       // Because in robot.xml file we already define a pose of bodybase wrt to world.
       // Then we need to combine this two pose together.
       void InitPoseOfBodyBaseWRTWorld(Eigen::Vector6d eRobotInitPoseInWorld)
       {
           m_eRobotInitPose = eRobotInitPoseInWorld - m_eRobotInitPoseInURDF;
       }


       // set pose of body base in URDF as initial pose
       void InitPoseOfBodyBaseInURDF()
       {
           m_eRobotInitPose = m_eRobotInitPoseInURDF;
       }



       // -----------------------------------------------------------------------------------------------------------------
       // get the pose of robot base from model node
       Eigen::Vector6d GetRobotBasePose()
       {
           ModelNode* pNode= m_RobotModel.m_vChildren[0];
           Eigen::Vector6d pose= pNode->GetWPose();
           return pose;
       }



       // get name of all body that belong to the robot.
       vector<string> GetAllBodyName()
       {
           vector<string>  vBodyName;
           vector<string>  vAllBodyNameList = m_rPhyMGAgent.m_Agent.GetPhys()->GetAllEntityName();
           for(unsigned int i=0;i!=vAllBodyNameList.size();i++)
           {
               string sFullName= vAllBodyNameList[i];
               if(GetRobotNameFromFullName(sFullName)== m_sRobotName )
               {
                   vBodyName.push_back(sFullName);
               }
           }

           return vBodyName;
       }



       // -----------------------------------------------------------------------------------------------------------------
       // select between PD or PID control
       enum InitPoseMethod{
            FromXML = 1,
            FromServer = 2
       };


       // -----------------------------------------------------------------------------------------------------------------
       string GetRobotName()
       {
            return m_sRobotName;
       }


       // -----------------------------------------------------------------------------------------------------------------
       XMLDocument* GetRobotURDF()
       {
            return m_pRobotURDF;
       }


       // -----------------------------------------------------------------------------------------------------------------

private:
       Render                      m_Render;
       PhyModelGraphAgent          m_rPhyMGAgent;
       ModelGraphBuilder           m_MGBuilder;
       string                      m_sRobotName;
       string                      m_sProxyName;
       Model                       m_RobotModel;           // model graph
       XMLDocument*                m_pRobotURDF;           // robot URDF file
       Eigen::Vector6d             m_eRobotInitPose;       // actual pose that init robot
       Eigen::Vector6d             m_eRobotInitPoseInURDF; // init pose in robot urdf file.
};

#endif