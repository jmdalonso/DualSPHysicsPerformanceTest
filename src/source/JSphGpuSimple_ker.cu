//HEAD_DSPH
/*
 <DUALSPHYSICS>  Copyright (c) 2020 by Dr Jose M. Dominguez et al. (see http://dual.sphysics.org/index.php/developers/). 

 EPHYSLAB Environmental Physics Laboratory, Universidade de Vigo, Ourense, Spain.
 School of Mechanical, Aerospace and Civil Engineering, University of Manchester, Manchester, U.K.

 This file is part of DualSPHysics. 

 DualSPHysics is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version. 

 DualSPHysics is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. 

 You should have received a copy of the GNU General Public License, along with DualSPHysics. If not, see <http://www.gnu.org/licenses/>. 
*/

/// \file JSphGpuSimple_ker.cu \brief Implements functions and CUDA kernels for the Particle Interaction and System Update.

#include "JSphGpuSimple_ker.h"
//#include "Functions.h"
//#include "FunctionsCuda.h"
//#include <math_constants.h>
//#include "JDgKerPrint.h"
//#include "JDgKerPrint_ker.h"


namespace cusphs{
#include "FunctionsBasic_iker.cu"


//##############################################################################
//# Kernels to prepare data before Interaction_Forces().
//##############################################################################
//------------------------------------------------------------------------------
/// Update PosCellg[] according to current position of particles.
/// Actualiza PosCellg[] segun la posicion de las particulas.
//------------------------------------------------------------------------------
__global__ void KerUpdatePosCell(unsigned np,double3 posmin,float dosh
  ,const double2 *posxy,const double *posz,float4 *poscell)
{
  const unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<np){
    const double2 rxy=posxy[p];
    const double dx=rxy.x-posmin.x;
    const double dy=rxy.y-posmin.y;
    const double dz=posz[p]-posmin.z;
    const unsigned cx=unsigned(dx/dosh);
    const unsigned cy=unsigned(dy/dosh);
    const unsigned cz=unsigned(dz/dosh);
    const float px=float(dx-(double(dosh)*cx));
    const float py=float(dy-(double(dosh)*cy));
    const float pz=float(dz-(double(dosh)*cz));
    const float pw=__uint_as_float(CEL_Code(cx,cy,cz));
    poscell[p]=make_float4(px,py,pz,pw);
  }
}
//==============================================================================
/// Update PosCellg[] according to current position of particles.
/// Actualiza PosCellg[] segun la posicion de las particulas.
//==============================================================================
void UpdatePosCell(unsigned np,tdouble3 posmin,float dosh
  ,const double2 *posxy,const double *posz,float4 *poscell,cudaStream_t stm)
{
  const dim3 sgrid=GetSimpleGridSize(np,SPHBSIZE);
  if(np)KerUpdatePosCell <<<sgrid,SPHBSIZE,0,stm>>> (np,Double3(posmin),dosh,posxy,posz,poscell);
}

//------------------------------------------------------------------------------
/// Initialises ace array with 0 for bound and gravity for fluid.
/// Inicializa el array ace con 0 para contorno y gravity para fluido.
//------------------------------------------------------------------------------
__global__ void KerInitAceGravity(unsigned np,unsigned npb,float3 gravity,float3 *ace)
{
  unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<np){
    ace[p]=(p<npb? make_float3(0,0,0): gravity);
  }
}
//==============================================================================
/// Initialises ace array with 0 for bound and gravity for fluid.
/// Inicializa el array ace con 0 para contorno y gravity para fluido.
//==============================================================================
void InitAceGravity(unsigned np,unsigned npb,tfloat3 gravity,float3 *ace,cudaStream_t stm){
  if(np){
    dim3 sgrid=GetSimpleGridSize(np,SPHBSIZE);
    KerInitAceGravity <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,Float3(gravity),ace);
  }
}


//##############################################################################
//# Kernels to run after Interaction_Forces().
//##############################################################################
//------------------------------------------------------------------------------
/// Sets v[].y to zero.
/// Pone v[].y a cero.
//------------------------------------------------------------------------------
__global__ void KerResety(unsigned n,unsigned ini,float3 *v)
{
  unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<n)v[p+ini].y=0;
}
//==============================================================================
/// Sets v[].y to zero.
/// Pone v[].y a cero.
//==============================================================================
void Resety(unsigned n,unsigned ini,float3 *v,cudaStream_t stm){
  if(n){
    dim3 sgrid=GetSimpleGridSize(n,SPHBSIZE);
    KerResety <<<sgrid,SPHBSIZE,0,stm>>> (n,ini,v);
  }
}


//##############################################################################
//# Kernels for ComputeStep (vel & rhop).
//# Kernels para ComputeStep (vel & rhop).
//##############################################################################
//------------------------------------------------------------------------------
/// Computes new values for Pos, Check, Vel and Ros (using Verlet).
/// The value of Vel always set to be reset.
///
/// Calcula nuevos valores de  Pos, Check, Vel y Rhop (usando Verlet).
/// El valor de Vel para bound siempre se pone a cero.
//------------------------------------------------------------------------------
template<bool floating,bool shift,bool inout> __global__ void KerComputeStepVerlet
  (unsigned n,unsigned npb,float rhopzero,float rhopoutmin,float rhopoutmax
  ,const float4 *velrhop1,const float4 *velrhop2
  ,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dt,double dt205,double dt2,float3 gravity
  ,double2 *movxy,double *movz,typecode *code,float4 *velrhopnew)
{
  unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<n){
    if(p<npb){ //-Particles: Fixed & Moving.
      float rrhop=float(double(velrhop2[p].w)+dt2*ar[p]);
      rrhop=(rrhop<rhopzero? rhopzero: rrhop); //-To prevent absorption of fluid particles by boundaries. | Evita q las boundary absorvan a las fluidas.
      velrhopnew[p]=make_float4(0,0,0,rrhop);
    }
    else{ //-Particles: Floating & Fluid.
      //-Updates density.
      const float4 rvelrhop2=velrhop2[p];
      const float rhopnew=float(double(velrhop2[p].w)+dt2*ar[p]);
      float4 rvel1=velrhop1[p];
      if(!floating || CODE_IsFluid(code[p])){ //-Particles: Fluid.
        //-Calculate displacement. | Calcula desplazamiento.
        const float3 race=ace[p];
        const double acegrx=double(race.x)+gravity.x;
        const double acegry=double(race.y)+gravity.y;
        const double acegrz=double(race.z)+gravity.z;
        double dx=double(rvel1.x)*dt + acegrx*dt205;
        double dy=double(rvel1.y)*dt + acegry*dt205;
        double dz=double(rvel1.z)*dt + acegrz*dt205;
        if(shift){
          const float4 rshiftpos=shiftposfs[p];
          dx+=double(rshiftpos.x);
          dy+=double(rshiftpos.y);
          dz+=double(rshiftpos.z);
        }
        bool outrhop=(rhopnew<rhopoutmin || rhopnew>rhopoutmax);
        //-Calculate velocity & density. | Calcula velocidad y densidad.
        float4 rvelrhopnew=make_float4(
          float(double(rvelrhop2.x) + acegrx*dt2),
          float(double(rvelrhop2.y) + acegry*dt2),
          float(double(rvelrhop2.z) + acegrz*dt2),
          rhopnew);
        //-Restore data of inout particles.
        if(inout && CODE_IsFluidInout(code[p])){
          dx=double(rvel1.x)*dt;
          dy=double(rvel1.y)*dt;
          dz=double(rvel1.z)*dt;
          outrhop=false;
          rvelrhopnew=rvelrhop2;
        }
        //-Update particle data.
        movxy[p]=make_double2(dx,dy);
        movz[p]=dz;
        if(outrhop){ //-Only brands as excluded normal particles (not periodic). | Solo marca como excluidas las normales (no periodicas).
          const typecode rcode=code[p];
          if(CODE_IsNormal(rcode))code[p]=CODE_SetOutRhop(rcode);
        }
        velrhopnew[p]=rvelrhopnew;
      }
      else{ //-Particles: Floating.
        rvel1.w=(rhopnew<rhopzero? rhopzero: rhopnew); //-To prevent absorption of fluid particles by boundaries. | Evita q las floating absorvan a las fluidas.
        velrhopnew[p]=rvel1;
      }
    }
  }
}
//==============================================================================
/// Updates particles according to forces and dt using Verlet. 
/// Actualizacion de particulas segun fuerzas y dt usando Verlet.
//==============================================================================
void ComputeStepVerlet(bool floating,bool shift,bool inout,unsigned np,unsigned npb
  ,const float4 *velrhop1,const float4 *velrhop2
  ,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dt,double dt2,float rhopzero,float rhopoutmin,float rhopoutmax,tfloat3 gravity
  ,typecode *code,double2 *movxy,double *movz,float4 *velrhopnew,cudaStream_t stm)
{
  double dt205=(0.5*dt*dt);
  if(np){
    dim3 sgrid=GetSimpleGridSize(np,SPHBSIZE);
    if(inout){      const bool tinout=true;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepVerlet<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
        else        KerComputeStepVerlet<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
      }else{        const bool shift=false;
        if(floating)KerComputeStepVerlet<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
        else        KerComputeStepVerlet<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
      }
    }
    else{           const bool tinout=false;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepVerlet<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
        else        KerComputeStepVerlet<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
      }else{        const bool shift=false;
        if(floating)KerComputeStepVerlet<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
        else        KerComputeStepVerlet<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,rhopzero,rhopoutmin,rhopoutmax,velrhop1,velrhop2,ar,ace,shiftposfs,dt,dt205,dt2,Float3(gravity),movxy,movz,code,velrhopnew);
      }
    }
  }
}

//------------------------------------------------------------------------------
/// Computes new values for Pos, Check, Vel and Ros (used with Symplectic-Predictor).
/// Calcula los nuevos valores de Pos, Vel y Rhop (usando para Symplectic-Predictor).
//------------------------------------------------------------------------------
template<bool floating,bool shift,bool inout> __global__ void KerComputeStepSymplecticPre
  (unsigned n,unsigned npb
  ,const float4 *velrhoppre,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dtm,float rhopzero,float rhopoutmin,float rhopoutmax,float3 gravity
  ,typecode *code,double2 *movxy,double *movz,float4 *velrhop)
{
  unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<n){
    if(p<npb){ //-Particles: Fixed & Moving.
      float4 rvelrhop=velrhoppre[p];
      rvelrhop.w=float(double(rvelrhop.w)+dtm*ar[p]);
      rvelrhop.w=(rvelrhop.w<rhopzero? rhopzero: rvelrhop.w); //-To prevent absorption of fluid particles by boundaries. | Evita que las boundary absorvan a las fluidas.
      velrhop[p]=rvelrhop;
    }
    else{ //-Particles: Floating & Fluid.
      //-Updates density.
      const float4 rvelrhoppre=velrhoppre[p];
      float4 rvelrhopnew=rvelrhoppre;
      rvelrhopnew.w=float(double(rvelrhoppre.w)+dtm*ar[p]);
      if(!floating || CODE_IsFluid(code[p])){ //-Particles: Fluid.
        //-Calculate displacement. | Calcula desplazamiento.
        double dx=double(rvelrhoppre.x)*dtm;
        double dy=double(rvelrhoppre.y)*dtm;
        double dz=double(rvelrhoppre.z)*dtm;
        if(shift){
          const float4 rshiftpos=shiftposfs[p];
          dx+=double(rshiftpos.x);
          dy+=double(rshiftpos.y);
          dz+=double(rshiftpos.z);
        }
        bool outrhop=(rvelrhopnew.w<rhopoutmin || rvelrhopnew.w>rhopoutmax);
        //-Calculate velocity & density. | Calcula velocidad y densidad.
        const float3 race=ace[p];
        rvelrhopnew.x=float(double(rvelrhoppre.x) + (double(race.x)+gravity.x) * dtm);
        rvelrhopnew.y=float(double(rvelrhoppre.y) + (double(race.y)+gravity.y) * dtm);
        rvelrhopnew.z=float(double(rvelrhoppre.z) + (double(race.z)+gravity.z) * dtm);
        //-Restore data of inout particles.
        if(inout && CODE_IsFluidInout(code[p])){
          outrhop=false;
          rvelrhopnew=rvelrhoppre;
        }
        //-Update particle data.
        movxy[p]=make_double2(dx,dy);
        movz[p]=dz;
        if(outrhop){ //-Only brands as excluded normal particles (not periodic). | Solo marca como excluidas las normales (no periodicas).
          const typecode rcode=code[p];
          if(CODE_IsNormal(rcode))code[p]=CODE_SetOutRhop(rcode);
        }
      }
      else{ //-Particles: Floating.
        rvelrhopnew.w=(rvelrhopnew.w<rhopzero? rhopzero: rvelrhopnew.w); //-To prevent absorption of fluid particles by boundaries. | Evita q las floating absorvan a las fluidas.
      }
      //-Stores new velocity and density.
      velrhop[p]=rvelrhopnew;
    }
  }
}
//==============================================================================
/// Updates particles using Symplectic-Predictor.
/// Actualizacion de particulas usando Symplectic-Predictor.
//==============================================================================   
void ComputeStepSymplecticPre(bool floating,bool shift,bool inout,unsigned np,unsigned npb
  ,const float4 *velrhoppre,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dtm,float rhopzero,float rhopoutmin,float rhopoutmax,tfloat3 gravity
  ,typecode *code,double2 *movxy,double *movz,float4 *velrhop,cudaStream_t stm)
{
  if(np){
    dim3 sgrid=GetSimpleGridSize(np,SPHBSIZE);
    if(inout){      const bool tinout=true;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepSymplecticPre<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticPre<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }else{        const bool shift=false;
        if(floating)KerComputeStepSymplecticPre<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticPre<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }
    }
    else{           const bool tinout=false;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepSymplecticPre<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticPre<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }else{        const bool shift=false;
        if(floating)KerComputeStepSymplecticPre<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticPre<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }
    }
  }
}

//------------------------------------------------------------------------------
/// Computes new values for Pos, Check, Vel and Ros (using Verlet).
/// The value of Vel always set to be reset.
///
/// Calcula los nuevos valores de Pos, Vel y Rhop (usandopara Symplectic-Corrector).
/// Pone vel de contorno a cero.
//------------------------------------------------------------------------------
template<bool floating,bool shift,bool inout> __global__ void KerComputeStepSymplecticCor
  (unsigned n,unsigned npb
  ,const float4 *velrhoppre,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dtm,double dt,float rhopzero,float rhopoutmin,float rhopoutmax,float3 gravity
  ,typecode *code,double2 *movxy,double *movz,float4 *velrhop)
{
  unsigned p=blockIdx.x*blockDim.x + threadIdx.x; //-Number of particle.
  if(p<n){
    if(p<npb){ //-Particles: Fixed & Moving.
      double epsilon_rdot=(-double(ar[p])/double(velrhop[p].w))*dt;
      float rrhop=float(double(velrhoppre[p].w) * (2.-epsilon_rdot)/(2.+epsilon_rdot));
      rrhop=(rrhop<rhopzero? rhopzero: rrhop); //-To prevent absorption of fluid particles by boundaries. | Evita q las boundary absorvan a las fluidas.
      velrhop[p]=make_float4(0,0,0,rrhop);
    }
    else{ //-Particles: Floating & Fluid.
      //-Updates density.
      const double epsilon_rdot=(-double(ar[p])/double(velrhop[p].w))*dt;
      const float4 rvelrhoppre=velrhoppre[p];
      float4 rvelrhopnew=rvelrhoppre;
      rvelrhopnew.w=float(double(rvelrhoppre.w) * (2.-epsilon_rdot)/(2.+epsilon_rdot));
      if(!floating || CODE_IsFluid(code[p])){//-Particles: Fluid.
        //-Calculate velocity. | Calcula velocidad.
        const float3 race=ace[p];
        rvelrhopnew.x=float(double(rvelrhoppre.x) + (double(race.x)+gravity.x) * dt);
        rvelrhopnew.y=float(double(rvelrhoppre.y) + (double(race.y)+gravity.y) * dt);
        rvelrhopnew.z=float(double(rvelrhoppre.z) + (double(race.z)+gravity.z) * dt);
        //-Calculate displacement. | Calcula desplazamiento.
        double dx=(double(rvelrhoppre.x)+double(rvelrhopnew.x)) * dtm;
        double dy=(double(rvelrhoppre.y)+double(rvelrhopnew.y)) * dtm;
        double dz=(double(rvelrhoppre.z)+double(rvelrhopnew.z)) * dtm;
        if(shift){
          const float4 rshiftpos=shiftposfs[p];
          dx+=double(rshiftpos.x);
          dy+=double(rshiftpos.y);
          dz+=double(rshiftpos.z);
        }
        bool outrhop=(rvelrhopnew.w<rhopoutmin || rvelrhopnew.w>rhopoutmax);
        //-Restore data of inout particles.
        if(inout && CODE_IsFluidInout(code[p])){
          rvelrhopnew=rvelrhoppre;
          dx=(double(rvelrhopnew.x)+double(rvelrhopnew.x)) * dtm; 
          dy=(double(rvelrhopnew.y)+double(rvelrhopnew.y)) * dtm; 
          dz=(double(rvelrhopnew.z)+double(rvelrhopnew.z)) * dtm;
          outrhop=false;
        }
        //-Update particle data.
        movxy[p]=make_double2(dx,dy);
        movz[p]=dz;
        if(outrhop){ //-Only brands as excluded normal particles (not periodic). | Solo marca como excluidas las normales (no periodicas).
          const typecode rcode=code[p];
          if(CODE_IsNormal(rcode))code[p]=CODE_SetOutRhop(rcode);
        }
      }
      else{ //-Particles: Floating.
        rvelrhopnew.w=(rvelrhopnew.w<rhopzero? rhopzero: rvelrhopnew.w); //-To prevent absorption of fluid particles by boundaries. | Evita q las floating absorvan a las fluidas.
      }
      //-Stores new velocity and density.
      velrhop[p]=rvelrhopnew;
    }
  }
}
//==============================================================================
/// Updates particles using Symplectic-Corrector.
/// Actualizacion de particulas usando Symplectic-Corrector.
//==============================================================================   
void ComputeStepSymplecticCor(bool floating,bool shift,bool inout,unsigned np,unsigned npb
  ,const float4 *velrhoppre,const float *ar,const float3 *ace,const float4 *shiftposfs
  ,double dtm,double dt,float rhopzero,float rhopoutmin,float rhopoutmax,tfloat3 gravity
  ,typecode *code,double2 *movxy,double *movz,float4 *velrhop,cudaStream_t stm)
{
  if(np){
    dim3 sgrid=GetSimpleGridSize(np,SPHBSIZE);
    if(inout){      const bool tinout=true;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepSymplecticCor<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticCor<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }else{        const bool shift=false;
        if(floating)KerComputeStepSymplecticCor<true ,shift,tinout>  <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticCor<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }
    }
    else{           const bool tinout=false;
      if(shift){    const bool shift=true;
        if(floating)KerComputeStepSymplecticCor<true ,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticCor<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }else{        const bool shift=false;
        if(floating)KerComputeStepSymplecticCor<true ,shift,tinout>  <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
        else        KerComputeStepSymplecticCor<false,shift,tinout> <<<sgrid,SPHBSIZE,0,stm>>> (np,npb,velrhoppre,ar,ace,shiftposfs,dtm,dt,rhopzero,rhopoutmin,rhopoutmax,Float3(gravity),code,movxy,movz,velrhop);
      }
    }
  }
}


}


