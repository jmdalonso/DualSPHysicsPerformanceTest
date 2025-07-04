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

//:#############################################################################
//:# Cambios:
//:# =========
//:# - Clase para medir magnitudes fisicas durante la simulacion. (12-02-2018)
//:# - Se escriben las unidades en las cabeceras de los ficheros CSV. (26-04-2018)
//:# - Gestion de excepciones mejorada.  (15-09-2019)
//:#############################################################################

/// \file JGaugeItem.h \brief Declares the class \ref JGaugeItem.

#ifndef _JGaugeItem_
#define _JGaugeItem_

#include <string>
#include <vector>
#include "JObject.h"
#include "DualSphDef.h"
#include "JSaveCsv2.h"
#ifdef _WITHGPU
  #include <cuda_runtime_api.h>
#endif

//-Defines for CUDA exceptions.
#ifdef _WITHGPU
#ifndef Run_ExceptioonCuda
#define Run_ExceptioonCuda(cuerr,msg) RunExceptioonCuda(__FILE__,__LINE__,ClassName,__func__,cuerr,msg)
#endif
#ifndef Check_CudaErroor
#define Check_CudaErroor(msg) CheckCudaErroor(__FILE__,__LINE__,ClassName,__func__,msg)
#endif
#endif

class JLog2;

//##############################################################################
//# JGaugeItem
//##############################################################################
/// \brief Defines the common part of different gauge types.

class JGaugeItem : protected JObject
{
protected:
 #ifdef _WITHGPU
  void RunExceptioonCuda(const std::string &srcfile,int srcline
    ,const std::string &classname,const std::string &method
    ,cudaError_t cuerr,std::string msg)const;
  void CheckCudaErroor(const std::string &srcfile,int srcline
    ,const std::string &classname,const std::string &method
    ,std::string msg)const;
 #endif

public:

  ///Types of gauges.
  typedef enum{ 
     GAUGE_Vel
    ,GAUGE_Swl
    ,GAUGE_MaxZ
    ,GAUGE_Force
  }TpGauge;

  ///Structure with default configuration for JGaugeItem objects.
  typedef struct{
    bool savevtkpart;
    double computedt;
    double computestart;
    double computeend;
    bool output;
    double outputdt;
    double outputstart;
    double outputend;
  }StDefault;

protected:
  JLog2* Log;
  const bool Cpu;
  std::string FileInfo;

  //-Variables for calculation (they are constant).
  bool Simulate2D;     ///<Toggles 2D simulation (cancels forces in Y axis). | Activa o desactiva simulacion en 2D (anula fuerzas en eje Y).
  bool Symmetry;       ///<Use of symmetry in plane y=0.
  tdouble3 DomPosMin;  ///<Lower limit of simulation + edge 2h if periodic conditions. DomPosMin=Map_PosMin+(DomCelIni*Scell); | Limite inferior de simulacion + borde 2h si hay condiciones periodicas. 
  tdouble3 DomPosMax;  ///<Upper limit of simulation + edge 2h if periodic conditions. DomPosMax=min(Map_PosMax,Map_PosMin+(DomCelFin*Scell)); | Limite inferior de simulacion + borde 2h si hay condiciones periodicas. 
  float Scell;         ///<Cell size: 2h or h. | Tamanho de celda: 2h o h.
  int Hdiv;            ///<Value to divide 2H. | Valor por el que se divide a DosH
  float H;             ///<The smoothing length [m].
  float Fourh2;        ///<Constant related to H (Fourh2=H*H*4).
  float Awen;          ///<Wendland kernel constant (awen) to compute wab.
  float Bwen;          ///<Wendland kernel constant (bwen) to compute fac (kernel derivative).
  float MassFluid;     ///<Reference mass of the fluid particle [kg].
  float MassBound;     ///<Reference mass of the general boundary particle [kg].
  float Cs0;           ///<Speed of sound at the reference density.
  float CteB;          ///<Constant used in the state equation [Pa].
  float Gamma;         ///<Politropic constant for water used in the state equation.
  float RhopZero;      ///<Reference density of the fluid [kg/m3].

  //-Configuration variables.
  bool SaveVtkPart; //-Creates VTK files for each PART.
  double ComputeDt;
  double ComputeStart;
  double ComputeEnd;
  double ComputeNext;
  bool OutputSave;
  double OutputDt;
  double OutputStart;
  double OutputEnd;
  double OutputNext;

  //-Results of measurement.
  double TimeStep;

  //-Variables to store the results in buffer.
  static const unsigned OutSize=200; ///<Maximum number of results in buffer.
  unsigned OutCount;                 ///<Number of stored results in buffer.
  std::string OutFile;

  JGaugeItem(TpGauge type,unsigned idx,std::string name,bool cpu,JLog2* log);
  void Reset();
  void SetTimeStep(double timestep);

  bool PointIsOut(double px,double py,double pz)const{ return(px!=px || py!=py || pz!=pz || px<DomPosMin.x || py<DomPosMin.y || pz<DomPosMin.z || px>=DomPosMax.x || py>=DomPosMax.y || pz>=DomPosMax.z); }
  bool PointIsOut(double px,double py)const{ return(px!=px || py!=py || px<DomPosMin.x || py<DomPosMin.y || px>=DomPosMax.x || py>=DomPosMax.y); }
  inline void GetInteractionCells(const tdouble3 &pos,const tint4 &nc,const tint3 &cellzero
    ,int &cxini,int &cxfin,int &yini,int &yfin,int &zini,int &zfin)const;
  inline float ComputePress(float rhop,float rhop0,float b,float gamma)const;

  static std::string GetNameType(TpGauge type);

  virtual void ClearResult()=0;
  virtual void StoreResult()=0;

public:
  const TpGauge Type;
  const unsigned Idx;
  const std::string Name;

  void Config(bool simulate2d,bool symmetry
    ,tdouble3 domposmin,tdouble3 domposmax
    ,float scell,int hdiv,float h,float massfluid,float massbound
    ,float cs0,float cteb,float gamma,float rhopzero);
  void SetSaveVtkPart(bool save){ SaveVtkPart=save; }
  void ConfigComputeTiming(double start,double end,double dt);
  void ConfigOutputTiming(bool save,double start,double end,double dt);

  void GetConfig(std::vector<std::string> &lines)const;

  std::string GetResultsFileCsv()const;
  std::string GetResultsFileVtk()const;
  virtual void SaveResults()=0;
  virtual void SaveVtkResult(unsigned cpart)=0;
  virtual unsigned GetPointDef(std::vector<tfloat3> &points)const=0;

  void SaveResults(unsigned cpart);

  double GetComputeDt()const{ return(ComputeDt); }
  double GetComputeStart()const{ return(ComputeStart); }
  double GetComputeEnd()const{ return(ComputeEnd); }

  double GetOutputDt()const{ return(OutputDt); }
  double GetOutputStart()const{ return(OutputStart); }
  double GetOutputEnd()const{ return(OutputEnd); }

  bool Update(double timestep)const{ return(timestep>=ComputeNext && ComputeStart<=timestep && timestep<=ComputeEnd); }
  bool Output(double timestep)const{ return(OutputSave && timestep>=OutputNext && OutputStart<=timestep && timestep<=OutputEnd); }

  virtual void CalculeCpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const unsigned *begincell,unsigned npbok,unsigned npb,unsigned np
    ,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop)=0;

 #ifdef _WITHGPU
  virtual void CalculeGpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const int2 *beginendcell,unsigned npbok,unsigned npb,unsigned np
    ,const double2 *posxy,const double *posz,const typecode *code,const unsigned *idp,const float4 *velrhop,float3 *aux)=0;
 #endif
};


//##############################################################################
//# JGaugeVelocity
//##############################################################################
/// \brief Calculates velocity in fluid domain.
class JGaugeVelocity : public JGaugeItem
{
public:
  ///Structure with result of JGaugeVelocity object.
  typedef struct StrGaugeVelRes{
    double timestep;
    tfloat3 point;
    tfloat3 vel;
    bool modified;
    StrGaugeVelRes(){ Reset(); }
    void Reset(){
      Set(0,TFloat3(0),TFloat3(0));
      modified=false;
    }
    void Set(double t,const tfloat3 &pt,const tfloat3 &ve){
      timestep=t; point=pt; vel=ve; modified=true;
    }
  }StGaugeVelRes;

protected:
  //-Definition.
  tdouble3 Point;

  StGaugeVelRes Result; ///<Result of the last measure.

  std::vector<StGaugeVelRes> OutBuff; ///<Results in buffer.

  void Reset();
  void ClearResult(){ Result.Reset(); }
  void StoreResult();

public:
  JGaugeVelocity(unsigned idx,std::string name,tdouble3 point,bool cpu,JLog2* log);
  ~JGaugeVelocity();

  void SaveResults();
  void SaveVtkResult(unsigned cpart);
  unsigned GetPointDef(std::vector<tfloat3> &points)const;

  tdouble3 GetPoint()const{ return(Point); }
  const StGaugeVelRes& GetResult()const{ return(Result); }

  void SetPoint(const tdouble3 &point){ ClearResult(); Point=point; }

  void CalculeCpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const unsigned *begincell,unsigned npbok,unsigned npb,unsigned np
    ,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop);

 #ifdef _WITHGPU
  void CalculeGpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const int2 *beginendcell,unsigned npbok,unsigned npb,unsigned np
    ,const double2 *posxy,const double *posz,const typecode *code,const unsigned *idp,const float4 *velrhop,float3 *aux);
 #endif
};


//##############################################################################
//# JGaugeSwl
//##############################################################################
/// \brief Calculates Surface Water Level in fluid domain.
class JGaugeSwl : public JGaugeItem
{
public:
  ///Structure with result of JGaugeVelocity object.
  typedef struct StrGaugeSwlRes{
    double timestep;
    tfloat3 point0;
    tfloat3 point2;
    tfloat3 posswl;
    bool modified;
    StrGaugeSwlRes(){ Reset(); }
    void Reset(){
      Set(0,TFloat3(0),TFloat3(0),TFloat3(0));
      modified=false;
    }
    void Set(double t,const tfloat3 &pt0,const tfloat3 &pt2,const tfloat3 &ps){
      timestep=t; point0=pt0; point2=pt2; posswl=ps; modified=true;
    }
  }StGaugeSwlRes;

protected:
  //-Definition.
  tdouble3 Point0;
  tdouble3 Point2;
  double PointDp;
  float MassLimit;
  //-Auxiliary variables.
  unsigned PointNp;
  tdouble3 PointDir;

  StGaugeSwlRes Result; ///<Result of the last measure.

  std::vector<StGaugeSwlRes> OutBuff; ///<Results in buffer.

  void Reset();
  void ClearResult(){ Result.Reset(); }
  void StoreResult();
  float CalculeMassCpu(const tdouble3 &ptpos,const tint4 &nc
    ,const tint3 &cellzero,unsigned cellfluid,const unsigned *begincell
    ,const tdouble3 *pos,const typecode *code,const tfloat4 *velrhop)const;

public:
  JGaugeSwl(unsigned idx,std::string name,tdouble3 point0,tdouble3 point2,double pointdp,float masslimit,bool cpu,JLog2* log);
  ~JGaugeSwl();

  void SaveResults();
  void SaveVtkResult(unsigned cpart);
  unsigned GetPointDef(std::vector<tfloat3> &points)const;

  tdouble3 GetPoint0()const{ return(Point0); }
  tdouble3 GetPoint2()const{ return(Point2); }
  double GetPointDp()const{ return(PointDp); }
  float GetMassLimit()const{ return(MassLimit); }
  const StGaugeSwlRes& GetResult()const{ return(Result); }

  void SetPoints(const tdouble3 &point0,const tdouble3 &point2,double pointdp);

  void CalculeCpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const unsigned *begincell,unsigned npbok,unsigned npb,unsigned np
    ,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop);

 #ifdef _WITHGPU
  void CalculeGpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const int2 *beginendcell,unsigned npbok,unsigned npb,unsigned np
    ,const double2 *posxy,const double *posz,const typecode *code,const unsigned *idp,const float4 *velrhop,float3 *aux);
 #endif
};


//##############################################################################
//# JGaugeMaxZ
//##############################################################################
/// \brief Calculates maximum z of fluid at distance of a vertical line.
class JGaugeMaxZ : public JGaugeItem
{
public:
  ///Structure with result of JGaugeMaxZ object.
  typedef struct StrGaugeMaxzRes{
    double timestep;
    tfloat3 point0;
    float zmax;
    bool modified;
    StrGaugeMaxzRes(){ Reset(); }
    void Reset(){
      Set(0,TFloat3(0),0);
      modified=false;
    }
    void Set(double t,const tfloat3 &pt,float z){
      timestep=t; point0=pt; zmax=z; modified=true;
    }
  }StGaugeMaxzRes;

protected:
  //-Definition.
  tdouble3 Point0;
  double Height;
  float DistLimit;

  StGaugeMaxzRes Result; ///<Result of the last measure.

  std::vector<StGaugeMaxzRes> OutBuff; ///<Results in buffer.

  void Reset();
  void ClearResult(){ Result.Reset(); }
  void StoreResult();
  void GetInteractionCellsMaxZ(const tdouble3 &pos,const tint4 &nc,const tint3 &cellzero
    ,int &cxini,int &cxfin,int &yini,int &yfin,int &zini,int &zfin)const;

public:
  JGaugeMaxZ(unsigned idx,std::string name,tdouble3 point0,double height,float distlimit,bool cpu,JLog2* log);
  ~JGaugeMaxZ();

  void SaveResults();
  void SaveVtkResult(unsigned cpart);
  unsigned GetPointDef(std::vector<tfloat3> &points)const;

  tdouble3 GetPoint0()const{ return(Point0); }
  double GetHeight()const{ return(Height); }
  float GetDistLimit()const{ return(DistLimit); }
  const StGaugeMaxzRes& GetResult()const{ return(Result); }

  void SetPoint0   (const tdouble3 &point0){ ClearResult(); Point0=point0; }
  void SetHeight   (double height){          ClearResult(); Height=height; }
  void SetDistLimit(float distlimit){        ClearResult(); DistLimit=distlimit; }

  void CalculeCpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const unsigned *begincell,unsigned npbok,unsigned npb,unsigned np
    ,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop);

 #ifdef _WITHGPU
  void CalculeGpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const int2 *beginendcell,unsigned npbok,unsigned npb,unsigned np
    ,const double2 *posxy,const double *posz,const typecode *code,const unsigned *idp,const float4 *velrhop,float3 *aux);
 #endif
};


//##############################################################################
//# JGaugeForce
//##############################################################################
/// \brief Calculates force sumation on selected particles (using only fluid particles).
class JGaugeForce : public JGaugeItem
{
public:
  ///Structure with result of JGaugeForce object.
  typedef struct StrGaugeForceRes{
    double timestep;
    word mkbound;
    tfloat3 force;
    bool modified;
    StrGaugeForceRes(){ Reset(); }
    void Reset(){
      Set(0,TFloat3(0));
      modified=false;
    }
    void Set(double t,const tfloat3 &forceres){
      timestep=t; force=forceres; modified=true;
    }
  }StGaugeForceRes;

protected:
  //-Definition.
  word MkBound;
  TpParticles TypeParts;
  unsigned IdBegin;
  unsigned Count;
  typecode Code;
  tfloat3 InitialCenter;

  //-Auxiliary variables.
  tfloat3 *PartAcec;
 #ifdef _WITHGPU
  float3 *PartAceg;
  float3 *Auxg;
 #endif

  StGaugeForceRes Result; ///<Result of the last measure.

  std::vector<StGaugeForceRes> OutBuff; ///<Results in buffer.

  void Reset();
  void ClearResult(){ Result.Reset(); }
  void StoreResult();

public:
  JGaugeForce(unsigned idx,std::string name,word mkbound,TpParticles typeparts
    ,unsigned idbegin,unsigned count,typecode code,tfloat3 center,bool cpu,JLog2* log);
  ~JGaugeForce();

  void SaveResults();
  void SaveVtkResult(unsigned cpart);
  unsigned GetPointDef(std::vector<tfloat3> &points)const;

  word        GetMkBound()  const{ return(MkBound); }
  TpParticles GetTypeParts()const{ return(TypeParts); }
  unsigned    GetIdBegin()  const{ return(IdBegin); }
  unsigned    GetCount()    const{ return(Count); }
  typecode    GetCode()     const{ return(Code); }
  tfloat3     GetInitialCenter()const{ return(InitialCenter); }
  const StGaugeForceRes& GetResult()const{ return(Result); }

  void CalculeCpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const unsigned *begincell,unsigned npbok,unsigned npb,unsigned np
    ,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop);

 #ifdef _WITHGPU
  void CalculeGpu(double timestep,tuint3 ncells,tuint3 cellmin
    ,const int2 *beginendcell,unsigned npbok,unsigned npb,unsigned np
    ,const double2 *posxy,const double *posz,const typecode *code,const unsigned *idp,const float4 *velrhop,float3 *aux);
 #endif
};


#endif


