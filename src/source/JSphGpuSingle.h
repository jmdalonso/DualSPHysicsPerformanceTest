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

/// \file JSphGpuSingle.h \brief Declares the class \ref JSphGpuSingle.

#ifndef _JSphGpuSingle_
#define _JSphGpuSingle_

#include "DualSphDef.h"
#include "JSphGpu.h"
#include <string>

class JCellDivGpuSingle;

//##############################################################################
//# JSphGpuSingle
//##############################################################################
/// \brief Defines the attributes and functions used only in Single-GPU implementation.

class JSphGpuSingle : public JSphGpu
{
protected:
  JCellDivGpuSingle* CellDivSingle;

  llong GetAllocMemoryCpu()const;  
  llong GetAllocMemoryGpu()const;  
  llong GetMemoryGpuNp()const;
  llong GetMemoryGpuNct()const;
  void UpdateMaxValues();
  void LoadConfig(JCfgRun *cfg);
  void ConfigDomain();

  void ResizeParticlesSize(unsigned newsize,float oversize,bool updatedivide);
  void RunPeriodic();
  void RunCellDivide(bool updateperiodic);
  void AbortBoundOut();

  void Interaction_Forces(TpInterStep interstep);
  void BoundCorrection(); //<vs_mddbc>

  double ComputeAceMax(float *auxmem);

  double ComputeStep(){ return(TStep==STEP_Verlet? ComputeStep_Ver(): ComputeStep_Sym()); }

  double ComputeStep_Ver();
  double ComputeStep_Sym();

  void UpdateFtObjs();
  void FtApplyImposedVel(float3 *ftoforcesresg)const; //<vs_fttvel>
  void RunFloating(double dt,bool predictor);
  void RunGaugeSystem(double timestep);

  void SaveData();
  void FinishRun(bool stop);

public:
  JSphGpuSingle();
  ~JSphGpuSingle();
  void Run(std::string appname,JCfgRun *cfg,JLog2 *log);

//<vs_innlet_ini>
//-Code for InOut in JSphGpuSingle_InOut.cpp
//--------------------------------------------
protected:
  void InOutInit(double timestepini);
  void InOutIgnoreFluidDef(const std::vector<unsigned> &mkfluidlist);
  void InOutCheckProximity(unsigned newnp);
  void InOutComputeStep(double stepdt);
  void InOutCalculeZsurf();
  void InOutExtrapolateData(unsigned inoutcount,const int *inoutpart);

  void BoundCorrectionData();
//<vs_innlet_end>
};

#endif


