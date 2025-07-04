//HEAD_DSPH
/*
 <DUALSPHYSICS>  Copyright (c) 2020 by Dr Jose M. Dominguez et al. (see http://dual.sphysics.org/index.php/developers/). 

 EPHYSLAB Environmental Physics Laboratory, Universidade de Vigo, Ourense, Spain.
 School of Mechanical, Aerospace and Civil Engineering, University of Manchester, Manchester, U.K.

 This file is part of DualSPHysics. 

 DualSPHysics is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License 
 as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
 
 DualSPHysics is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details. 

 You should have received a copy of the GNU Lesser General Public License along with DualSPHysics. If not, see <http://www.gnu.org/licenses/>. 
*/

//:NO_COMENTARIO
//:#############################################################################
//:# Cambios:
//:# =========
//:# - Gestiona la inizializacion inicial de las particulas (01-02-2017)
//:# - Implementa normales para cilindors (body & tank) y esfera (tank). (31-01-2020)
//:# - Limita calculo de normales con MaxDisteH. (31-01-2020)
//:# - Objeto JXml pasado como const para operaciones de lectura. (18-03-2020)  
//:# - Comprueba opcion active en elementos de primer y segundo nivel. (19-03-2020)  
//:# - Opcion para calcular boundary limit de forma automatica. (19-05-2020)  
//:#############################################################################

/// \file JSphInitialize.h \brief Declares the class \ref JSphInitialize.

#ifndef _JSphInitialize_
#define _JSphInitialize_

#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "JObject.h"
#include "TypesDef.h"

class JXml;
class TiXmlElement;

//##############################################################################
//# XML format in _FmtXML_Initialize.xml.
//##############################################################################

//##############################################################################
//# JSphInitializeOp
//##############################################################################
/// \brief Base clase for initialization of particle data.
class JSphInitializeOp : public JObject
{
public:
  ///Types of initializations.
  typedef enum{ 
    IT_FluidVel=1,
    IT_BoundNormalSet=2,       //<vs_mddbc>
    IT_BoundNormalPlane=3,     //<vs_mddbc>
    IT_BoundNormalSphere=4,    //<vs_mddbc>
    IT_BoundNormalCylinder=5,  //<vs_mddbc>
  }TpInitialize; 

  ///Structure with constant values needed for initialization tasks.
  typedef struct StrInitCt{
    float h;          ///<The smoothing length [m].
    float dp;         ///<Initial distance between particles [m].
    unsigned nbound;  ///<Initial number of boundary particles (fixed+moving+floating).
    StrInitCt(float h_,float dp_,unsigned nbound_){
      h=h_; dp=dp_; nbound=nbound_;
    }
  }StInitCt;

public:
  const TpInitialize Type;   ///<Type of particle.
  const StInitCt InitCt;     ///<Constant values needed for initialization tasks.

public:
  JSphInitializeOp(TpInitialize type,const char* name,StInitCt initct)
    :Type(type),InitCt(initct)
  { 
    ClassName=std::string("JSphInitializeOp_")+name;
  } 
  virtual ~JSphInitializeOp(){ DestructorActive=true; }
  virtual void ReadXml(const JXml *sxml,TiXmlElement* ele)=0;
  virtual void Run(unsigned np,unsigned npb,const tdouble3 *pos
    ,const unsigned *idp,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal)=0;
  virtual void GetConfig(std::vector<std::string> &lines)const=0;
  unsigned ComputeDomainMk(bool bound,word mktp,unsigned np,const word *mktype
  ,const unsigned *idp,const tdouble3 *pos,tdouble3 &posmin,tdouble3 &posmax)const;
};

//##############################################################################
//# JSphInitializeOp_FluidVel
//##############################################################################
/// Initializes velocity of fluid particles.
class JSphInitializeOp_FluidVel : public JSphInitializeOp
{
private:
  ///Controls profile of imposed velocity.
  typedef enum{ 
    TVEL_Constant=0,    ///<Velocity profile uniform.
    TVEL_Linear=1,      ///<Velocity profile linear.
    TVEL_Parabolic=2    ///<Velocity profile parabolic.
  }TpVelocity;
private:
  TpVelocity VelType;  ///<Type of velocity.
  std::string MkFluid;
  tfloat3 Direction;
  float Vel1,Vel2,Vel3;
  float Posz1,Posz2,Posz3;
public:
  JSphInitializeOp_FluidVel(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JSphInitializeOp(IT_FluidVel,"FluidVel",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset();
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//<vs_mddbc_ini>
//##############################################################################
//# JSphInitializeOp_BoundNormalSet
//##############################################################################
/// Initializes normals of boundary particles.
class JSphInitializeOp_BoundNormalSet : public JSphInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Normal;
public:
  JSphInitializeOp_BoundNormalSet(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JSphInitializeOp(IT_BoundNormalSet,"BoundNormalSet",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset(){ MkBound=""; Normal=TFloat3(0); }
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JSphInitializeOp_BoundNormalPlane
//##############################################################################
/// Initializes normals of boundary particles.
class JSphInitializeOp_BoundNormalPlane : public JSphInitializeOp
{
private:
  std::string MkBound;
  bool PointAuto;   ///<Point is calculated automatically accoding to normal configuration.
  float LimitDist;  ///<Minimun distance (Dp*vdp) between particles and boundary limit to calculate the point (default=0.5).
  tfloat3 Point;
  tfloat3 Normal;
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JSphInitializeOp_BoundNormalPlane(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JSphInitializeOp(IT_BoundNormalPlane,"BoundNormalPlane",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset(){ MkBound=""; PointAuto=false; LimitDist=0; Point=Normal=TFloat3(0); MaxDisteH=0; }
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JSphInitializeOp_BoundNormalSphere
//##############################################################################
/// Initializes normals of boundary particles.
class JSphInitializeOp_BoundNormalSphere : public JSphInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Center;
  float Radius;
  bool Inside;      ///<Boundary particles inside the sphere.
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JSphInitializeOp_BoundNormalSphere(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JSphInitializeOp(IT_BoundNormalSphere,"BoundNormalSphere",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset(){ MkBound=""; Center=TFloat3(0); MaxDisteH=Radius=0; Inside=true; }
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;
};  

//##############################################################################
//# JSphInitializeOp_BoundNormalCylinder
//##############################################################################
/// Initializes normals of boundary particles.
class JSphInitializeOp_BoundNormalCylinder : public JSphInitializeOp
{
private:
  std::string MkBound;
  tfloat3 Center1;
  tfloat3 Center2;
  float Radius;
  bool Inside;      ///<Boundary particles inside the cylinder.
  float MaxDisteH;  ///<Maximum distance to boundary limit. It uses H*distanceh (default=2).
public:
  JSphInitializeOp_BoundNormalCylinder(const JXml *sxml,TiXmlElement* ele,StInitCt initct)
    :JSphInitializeOp(IT_BoundNormalCylinder,"BoundNormalCylinder",initct){ Reset(); ReadXml(sxml,ele); }
  void Reset(){ MkBound=""; Center1=Center2=TFloat3(0); MaxDisteH=Radius=0; Inside=true; }
  void ReadXml(const JXml *sxml,TiXmlElement* ele);
  void Run(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;
};  
//<vs_mddbc_end>


//##############################################################################
//# JSphInitialize
//##############################################################################
/// \brief Manages the info of particles from the input XML file.
class JSphInitialize  : protected JObject
{
private:
  const bool BoundNormals;
  const JSphInitializeOp::StInitCt InitCt;  ///<Constant values needed for initialization tasks.
  std::vector<JSphInitializeOp*> Opes;

  void LoadFileXml(const std::string &file,const std::string &path);
  void LoadXml(const JXml *sxml,const std::string &place);
  void ReadXml(const JXml *sxml,TiXmlElement* lis);

public:
  JSphInitialize(const JXml *sxml,const std::string &place
    ,float h,float dp,unsigned nbound,bool boundnormals);
  ~JSphInitialize();
  void Reset();
  unsigned Count()const{ return(unsigned(Opes.size())); }

  void Run(unsigned np,unsigned npb,const tdouble3 *pos
    ,const unsigned *idp,const word *mktype,tfloat4 *velrhop,tfloat3 *boundnormal);
  void GetConfig(std::vector<std::string> &lines)const;

};

#endif


