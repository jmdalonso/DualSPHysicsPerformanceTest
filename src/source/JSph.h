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
//:# - El calculo de constantes en ConfigConstants() se hace usando double aunque
//:#   despues se convierte a float (22-04-2013)
//:# - Mejora la gestion de excepciones. (06-05-2020)
//:#############################################################################

/// \file JSph.h \brief Declares the class \ref JSph.

#ifndef _JSph_
#define _JSph_

#include "DualSphDef.h"
#include "JObject.h"
#include "JCfgRun.h"
#include "JLog2.h"
#include "JTimer.h"
#include <float.h>
#include <string>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iostream>
#include <fstream>

class JSphMk;
class JSphMotion;
class JPartData;
class JPartPData;
class JSphDtFixed;
class JSaveDt;
class JSphVisco;
class JWaveGen;
class JMLPistons;   //<vs_mlapiston>
class JRelaxZones;  //<vs_rzone>
class JSphAccInput;
class JSpaceParts;
class JPartDataBi4;
class JPartOutBi4Save;
class JPartFloatBi4Save;
class JPartsOut;
class JShifting;
class JDamping;
class JXml;
class JTimeOut;
class JGaugeSystem;
class JPartsLoad4;
class JSpacePartBlock;
class JChronoObjects;    //<vs_chroono>
class JMooredFloatings;  //<vs_moordyyn>
class JSphFtForcePoints; //<vs_moordyyn>
class JSphInOut;         //<vs_innlet>
class JSphBoundCorr;     //<vs_innlet>
class JSphPartsInit;
class JLinearValue;
class JSpaceEParms;
class JDataArrays;
class JNumexLib;

//##############################################################################
//# XML format of execution parameters in _FmtXML__Parameters.xml.
//##############################################################################

//##############################################################################
//# JSph
//##############################################################################
/// \brief Defines all the attributes and functions that CPU and GPU simulations share.

class JSph : protected JObject
{
public:
/// Structure with constants for the Cubic Spline kernel.
  typedef struct {
    float a1,a2,aa,a24,c1,d1,c2;
    float od_wdeltap;        ///<Parameter for tensile instability correction.  
  }StCubicCte;

/// Structure that saves extra information about the execution.
  typedef struct {
    double timesim;      ///<Seconds from the start of the simulation (after loading the initial data).                    | Segundos desde el inicio de la simulacion (despues de cargar los datos iniciales).
    unsigned nct;        ///<Number of cells used in the divide.                                                           | Numero de celdas usadas en el divide.                                                    
    unsigned npbin;      ///<Number of boundary particles within the area of the divide (includes periodic particles).     | Numero de particulas bound dentro del area del divide (incluye particulas periodicas).
    unsigned npbout;     ///<Number of boundary particles outside of the area of the divide (includes periodic particles). | Numero de particulas bound fuera del area del divide (incluye particulas periodicas).    
    unsigned npf;        ///<Number of fluid particles (includes periodic particles).                                      | Numero de particulas fluid (incluye particulas periodicas).                              
    unsigned npbper;     ///<Number of periodic boundary particles (inside and outside the area of the split).             | Numero de particulas bound periodicas (dentro y fuera del area del divide).              
    unsigned npfper;     ///<Number of periodic fluid particles.                                                           | Numero de particulas fluid periodicas.                                                   
    unsigned newnp;      ///<Number of new fluid particles (inlet conditions)                                              | Numero de nuevas particulas fluid (inlet conditions).                                    
    llong memorycpualloc;
    bool gpudata;
    llong memorynpalloc;
    llong memorynpused;
    llong memorynctalloc;
    llong memorynctused;
  }StInfoPartPlus;

private:
  //-Configuration variables to compute the case limits.
  //-Variables de configuracion para calcular el limite del caso.
  tdouble3 CfgDomainParticlesMin,CfgDomainParticlesMax;
  tdouble3 CfgDomainParticlesPrcMin,CfgDomainParticlesPrcMax;
  tdouble3 CfgDomainFixedMin,CfgDomainFixedMax;

  //-Object for saving particles and information in files.
  //-Objeto para la grabacion de particulas e informacion en ficheros.
  JPartDataBi4 *DataBi4;            ///<To store particles and info in bi4 format.      | Para grabar particulas e info en formato bi4.
  JPartOutBi4Save *DataOutBi4;      ///<To store excluded particles in bi4 format.      | Para grabar particulas excluidas en formato bi4.
  JPartFloatBi4Save *DataFloatBi4;  ///<To store floating data in bi4 format.           | Para grabar datos de floatings en formato bi4.

  //-Total number of excluded particles according to reason for exclusion.
  //-Numero acumulado de particulas excluidas segun motivo.
  unsigned OutPosCount,OutRhopCount,OutMoveCount;

  void InitVars();
  std::string CalcRunCode()const;
  void AddOutCount(unsigned outpos,unsigned outrhop,unsigned outmove){ OutPosCount+=outpos; OutRhopCount+=outrhop; OutMoveCount+=outmove; }
  void ClearCfgDomain();
  void ConfigDomainFixed(tdouble3 vmin,tdouble3 vmax);
  void ConfigDomainFixedValue(std::string key,double v);
  void ConfigDomainParticles(tdouble3 vmin,tdouble3 vmax);
  void ConfigDomainParticlesValue(std::string key,double v);
  void ConfigDomainParticlesPrc(tdouble3 vmin,tdouble3 vmax);
  void ConfigDomainParticlesPrcValue(std::string key,double v);
  void ConfigDomainResize(std::string key,const JSpaceEParms *eparms);

protected:
  const bool Cpu;
  const bool Mgpu;
  const bool WithMpi;
  JLog2 *Log;

  bool Simulate2D;       ///<Toggles 2D simulation (cancels forces in Y axis). | Activa o desactiva simulacion en 2D (anula fuerzas en eje Y).
  double Simulate2DPosY; ///<Y value in 2D simulations.                        | Valor de Y en simulaciones 2D.
  bool Symmetry;         ///<Activates symmetry in plane y=0 (default=false).
  bool Stable;
  bool SvPosDouble;      ///<Indicates whether Pos is saved as double in bi4 files. | Indica si en los ficheros bi4 se guarda Pos como double.

  std::string AppName;
  std::string Hardware;
  std::string RunCode;
  std::string RunTimeDate;
  std::string CaseName,DirCase,RunName;
  std::string DirOut;         ///<Specifies the general output directory.
  std::string DirDataOut;     ///<Specifies the output subdirectory for binary data.
  std::string FileXml;

  //-Options for execution.
  TpStep TStep;               ///<Step Algorithm: Verlet or Symplectic.                                  | Algoritmo de paso: Verlet o Symplectic.
  int VerletSteps;            ///<Number of steps to apply Eulerian equations.

  TpKernel TKernel;           ///<Kernel type: Cubic, Wendland or Gaussian.                              | Tipo de kernel: Cubic, Wendland o Gaussian.
  float Awen;                 ///<Wendland kernel constant (awen) to compute wab.                        | Constante para calcular wab con kernel Wendland.
  float Bwen;                 ///<Wendland kernel constant (bwen) to compute fac (kernel derivative).    | Constante para calcular fac (derivada del kernel) con kernel Wendland.
  float Agau;                 ///<Gaussian kernel constant to compute wab.                               | Constante para calcular wab con kernel Gaussian.
  float Bgau;                 ///<Gaussian kernel constant to compute fac (kernel derivative).           | Constante para calcular fac (derivada del kernel) con kernel Gaussian.
  StCubicCte CubicCte;        ///<Constants for Cubic Spline Kernel.                                     | Constante para kernel cubic spline.

  TpDensity TDensity;         ///<Density Diffusion Term 0:None, 1:Molteni, 2:Fourtakas, 3:Fourtakas(full) (default=0)
  float DDTValue;             ///<Value used with Density Diffusion Term (default=0.1)
  bool DDTArray;              ///<Use extra array to compute Density Diffusion Term. The correction is applied after particle interaction. 

  TpVisco TVisco;             ///<Viscosity type: Artificial,...                                         | Tipo de viscosidad: Artificial,...
  float Visco;  
  float ViscoBoundFactor;     ///<For boundary interaction use Visco*ViscoBoundFactor.                  | Para interaccion con contorno usa Visco*ViscoBoundFactor.
  JSphVisco *ViscoTime;       ///<Provides a viscosity value as a function of simulation time.          | Proporciona un valor de viscosidad en funcion del instante de la simulacion.

  TpBoundary TBoundary;       ///<Boundary condition: DBC, M-DBC.
  TpSlipMode SlipMode;        ///<Slip mode for mDBC 1:DBC vel=0, 2:No-slip, 3:Free slip (default=1).     //<vs_mddbc>
  bool MdbcCorrector;         ///<mDBC correction is also applied in corrector of Symplectic (default=0). //<vs_mddbc>
  float MdbcThreshold;        ///<Kernel support limit to apply mDBC correction (default=0).              //<vs_mddbc>
  bool UseNormals;            ///<Indicates use of normals for mDBC.                                      //<vs_mddbc>
  bool UseNormalsFt;          ///<Indicates use of normals of floating bodies for mDBC.                   //<vs_mddbc>
  bool SvNormals;             ///<Saves normals VTK each PART (for debug).                                //<vs_mddbc>

  bool RhopOut;               ///<Indicates whether the RhopOut density correction is active or not.    | Indica si activa la correccion de densidad RhopOut o no.                       
  float RhopOutMin;           ///<Minimum limit for Rhopout correction.                                 | Limite minimo para la correccion de RhopOut.
  float RhopOutMax;           ///<Maximum limit for Rhopout correction.                                 | Limite maximo para la correccion de RhopOut.

  double TimeMax;             ///<Time of simulation.
  double TimePart;            ///<Time of data.
  JTimeOut *TimeOut;
  int NstepsBreak;           ///<Maximum number of steps allowed (debug).
  bool SvAllSteps;           ///<Saves a PART for each step (debug).

  double DtIni;              ///<Initial Dt
  double DtMin;              ///<Minimum allowed Dt (if the calculated value is lower is replaced by DTmin).
  float CoefDtMin;           ///<Coefficient to calculate minimum time step. dtmin=coefdtmin*h/speedsound (def=0.03).
  bool DtAllParticles;       ///<Velocity of particles used to calculate DT. 1:All, 0:Only fluid/floating (def=0).
  JSphDtFixed *DtFixed;
  JSaveDt *SaveDt;

  float PartsOutMax;         ///<Allowed percentage of fluid particles out of the domain. | Porcentaje maximo de particulas excluidas permitidas.                                  
  unsigned NpMinimum;        ///<Minimum number of particles allowed.                     | Numero minimo de particulas permitidas.                                                
  unsigned PartsOutWrn;      ///<Limit percentage for warning generation about number of excluded particles in one PART.
  unsigned PartsOutTotWrn;   ///<Limit percentage for warning generation about total excluded particles.

  //-Configuration for result output.
  bool CsvSepComa;           ///<Separator character in CSV files (0=semicolon, 1=coma).
  byte SvData;               ///<Combination of the TpSaveDat values.                            | Combinacion de valores TpSaveDat.                                                      
  bool SvRes;                ///<Creates file with execution summary.                            | Graba fichero con resumen de ejecucion.
  bool SvTimers;             ///<Computes the time for each process.                             | Obtiene tiempo para cada proceso.
  bool SvDomainVtk;          ///<Stores VTK file with the domain of particles of each PART file. | Graba fichero vtk con el dominio de las particulas en cada Part. 
  //bool SvInterCount;       ///<Computes and saves number of interactions.                      | Calcula y graba el numero de interacciones.

  //-Constants for computation.
  float H;                 ///<The smoothing length [m].
  float CteB;              ///<Constant used in the state equation [Pa].
  float Gamma;             ///<Politropic constant for water used in the state equation.
  float CFLnumber;         ///<Coefficient to multiply dt.
  float RhopZero;          ///<Reference density of the fluid [kg/m3].
  double Dp;               ///<Initial distance between particles [m].
  double Cs0;              ///<Speed of sound at the reference density.
  float DDT2h;             ///<Constant for DDT1 & DDT2. DDT2h=DDTValue*2*H
  float DDTgz;             ///<Constant for DDT2.        DDTgz=RhopZero*Gravity.z/CteB
  float MassFluid;         ///<Reference mass of the fluid particle [kg].
  float MassBound;         ///<Reference mass of the general boundary particle [kg].
  tfloat3 Gravity;         ///<Gravitational acceleration [m/s^2].
  float Dosh;              ///<Maximum interaction distance between particles (Dosh=H+H) [m].
  float H2;                ///<Constant related to H (H2=H*H).
  float Fourh2;            ///<Constant related to H (Fourh2=H*H*4).
  float Eta2;              ///<Constant related to H (Eta2=(h*0.1)*(h*0.1)).
  float SpsSmag;           ///<Smagorinsky constant used in SPS turbulence model.
  float SpsBlin;           ///<Blin constant used in the SPS turbulence model.

  //-General information about case.
  tdouble3 CasePosMin;       ///<Lower particle limit of the case in the initial instant. | Limite inferior de particulas del caso en instante inicial.
  tdouble3 CasePosMax;       ///<Upper particle limit of the case in the initial instant. | Limite superior de particulas del caso en instante inicial.
  unsigned CaseNp;           ///<Number of total particles of initial PART.  
  unsigned CaseNfixed;       ///<Number of fixed boundary particles. 
  unsigned CaseNmoving;      ///<Number of moving boundary particles. 
  unsigned CaseNfloat;       ///<Number of floating boundary particles. 
  unsigned CaseNfluid;       ///<Number of fluid particles (including the excluded ones). 
  unsigned CaseNbound;       ///<Number of boundary particles ( \ref Nfixed + \ref Nmoving + \ref Nfloat ).
  unsigned CaseNpb;          ///<Number of particles of the boundary block ( \ref Nbound - \ref Nfloat ) or ( \ref Nfixed + \ref Nmoving).

  JSphMk *MkInfo;            ///<Stores information for the Mk of the particles.
  JSphPartsInit *PartsInit;  ///<Stores initial particles data for automatic configurations.

  //-Variables for periodic conditions.
  byte PeriActive;
  bool PeriX,PeriY,PeriZ;
  tdouble3 PeriXinc;    ///<Value that is added at the outer limit to modify the position.
  tdouble3 PeriYinc;    ///<Value that is added at the outer limit to modify the position.
  tdouble3 PeriZinc;    ///<Value that is added at the outer limit to modify the position.

  //-Variables to restart simulation.
  std::string PartBeginDir;   ///<Searches directory for starting PART.                   | Directorio donde busca el PART de arranque.
  unsigned PartBegin;         ///<Indicates the start (0: no resumption).                 | Indica el PART de arranque (0:Sin reanudacion).
  unsigned PartBeginFirst;    ///<Indicates the number of the first PART to be generated. | Indica el numero del primer PART a generar.                                    
  double PartBeginTimeStep;   ///<initial instant of the simulation                       | Instante de inicio de la simulación.                                          
  ullong PartBeginTotalNp;    ///<Total number of simulated particles.

  JPartsOut *PartsOut;        ///<Stores excluded particles until they are saved. | Almacena las particulas excluidas hasta su grabacion.
  bool WrnPartsOut;           ///<Active warning according to number of out particles (default=1).

  //-Variables for predefined movement.
  JSphMotion *SphMotion;      ///<Manages moving objects. It is NULL when there are not moving objects.

  //-Variables for floating bodies.
  StFloatingData *FtObjs;      ///<Data of floating objects. [FtCount]
  unsigned FtCount;            ///<Number of floating objects.
  float FtPause;               ///<Time to start floating bodies movement.
  TpFtMode FtMode;             ///<Defines interaction mode for floatings and boundaries.
  bool FtConstraints;          ///<Some floating motion constraint is defined.
  JLinearValue **FtLinearVel;  ///<Imposed linear velocity [FtCount].  //<vs_fttvel>
  JLinearValue **FtAngularVel; ///<Imposed angular velocity [FtCount]. //<vs_fttvel>
  bool FtIgnoreRadius;         ///<Ignores floating body radius with periodic boundary conditions (def=false).
  bool WithFloating;

  //-Variables for DEM (DEM).
  bool UseDEM;         ///<Use DEM for boundary collisions.
  static const unsigned DemDataSize=CODE_TYPE_FLUID;
  StDemData *DemData;  ///<Data of DEM objects. [DemDataSize]

  //<vs_chroono_ini>
  //-Variables for Chrono use.
  bool UseChrono;  ///<Use Chrono library for rigid body dynamics.
  JChronoObjects *ChronoObjects;  ///<Object for integration with Chrono Engine.
  //<vs_chroono_end>

  //<vs_moordyyn_ini>
  JMooredFloatings* Moorings;     ///<Manages floating bodies with moorings. | Gestiona floating bodies con amarres.
  JSphFtForcePoints* ForcePoints; ///<Manages forces to apply on floating bodies.
  //<vs_moordyyn_end>

  std::vector<std::string> InitializeInfo; ///<Stores information about initialize configuration applied.

  JNumexLib *NuxLib;            ///<Object to evaluate user-defined expressions in XML.

  JGaugeSystem *GaugeSystem;    ///<Object for automatic gauge system.

  JWaveGen *WaveGen;            ///<Object for wave generation.

  JMLPistons *MLPistons;        ///<Object for Multi-Layer Pistons.   //<vs_mlapiston>

  JRelaxZones *RelaxZones;      ///<Object for wave generation using Relaxation Zone (RZ).  //<vs_rzone>

  JShifting *Shifting;          ///<Object for shifting correction.
  TpShifting ShiftingMode;      ///<Mode of Shifting: None, NoBound, NoFixed, Full.

  JDamping *Damping;            ///<Object for damping zones.

  JSphAccInput *AccInput;  ///<Object for variable acceleration functionality.

  JSphInOut *InOut;         ///<Object for inlet/outlet conditions.  //<vs_innlet> 
  JSphBoundCorr *BoundCorr; ///<Object for boundary extrapolated correction (used in combination with InOut).  //<vs_innlet>

  //-Variables for division in cells.
  TpCellMode CellMode;     ///<Cell division mode. | Modo de division en celdas.
  unsigned Hdiv;           ///<Value to divide 2H. | Valor por el que se divide a DosH
  float Scell;             ///<Cell size: 2h or h. | Tamanho de celda: 2h o h.
  float MovLimit;          ///<Maximum distance a particle is allowed to move in one step (Scell*0.9) | Distancia maxima que se permite recorrer a una particula en un paso (Scell*0.9).

  //-Defines global domain of the simulation.
  tdouble3 MapRealPosMin;  ///<Real lower limit of simulation (without the periodic condition borders). MapRealPosMin=CasePosMin-(H*BORDER_MAP) | Limite inferior real de simulacion (sin bordes de condiciones periodicas).
  tdouble3 MapRealPosMax;  ///<Real upper limit of simulation (without the periodic condition borders). MapRealPosMax=CasePosMax+(H*BORDER_MAP) | Limite superior real de simulacion (sin bordes de condiciones periodicas).
  tdouble3 MapRealSize;    ///<Result of MapRealSize = MapRealPosMax - MapRealPosMin

  tdouble3 Map_PosMin;     ///<Lower limit of simulation + edge 2h if periodic conditions. Map_PosMin=MapRealPosMin-dosh(in periodic axis) | Limite inferior de simulacion + borde 2h si hay condiciones periodicas.
  tdouble3 Map_PosMax;     ///<Upper limit of simulation + edge 2h if periodic conditions. Map_PosMax=MapRealPosMax+dosh(in periodic axis) | Limite superior de simulacion + borde 2h si hay condiciones periodicas.
  tdouble3 Map_Size;       ///<Result of Map_Size = Map_PosMax - Map_PosMin
  tuint3 Map_Cells;        ///<Maximum number of cells within case limits. Map_Cells=TUint3(unsigned(ceil(Map_Size.xyz/Scell))             | Numero de celdas maximo segun los limites del caso.

  //-Local domain of the simualtion.
  //-Dominio local de la simulacion.
  tuint3 DomCelIni;        ///<First cell within the Map defining local simulation area. DomCelIni=TUint3(0) for Single-CPU | Celda inicial dentro de Map que define el area de simulacion local.
  tuint3 DomCelFin;        ///<Last cell within the Map defining local simulation area. DomCelIni=Map_Cells for Single-CPU  | Celda final dentro de Map que define el area de simulacion local.
  tuint3 DomCells;         ///<Number of cells in each direction. DomCells=DomCelFin-DomCelIni                              | Numero de celdas en cada direccion.                                                                

  tdouble3 DomPosMin;      ///<Lower limit of simulation + edge 2h if periodic conditions. DomPosMin=Map_PosMin+(DomCelIni*Scell); | Limite inferior de simulacion + borde 2h si hay condiciones periodicas. 
  tdouble3 DomPosMax;      ///<Upper limit of simulation + edge 2h if periodic conditions. DomPosMax=min(Map_PosMax,Map_PosMin+(DomCelFin*Scell)); | Limite inferior de simulacion + borde 2h si hay condiciones periodicas. 
  tdouble3 DomSize;        ///<Result of DomSize = DomPosMax - DomPosMin

  tdouble3 DomRealPosMin;  ///<Real lower limit of the simulation according to DomCelIni/Fin (without periodic condition borders) DomRealPosMin=max(DomPosMin,MapRealPosMin) | Limite real inferior de simulacion segun DomCelIni/Fin (sin bordes de condiciones periodicas).
  tdouble3 DomRealPosMax;  ///<Real upper limit of the simulation according to DomCelIni/Fin (without periodic condition borders) DomRealPosMax=min(DomPosMax,MapRealPosMax) | Limite real superior de simulacion segun DomCelIni/Fin (sin bordes de condiciones periodicas).
  unsigned DomCellCode;    ///<Key for encoding cell position within the Domain. | Clave para la codificacion de la celda de posicion dentro de Domain.

  //-Controls particle number.
  bool NpDynamic;          ///<CaseNp can increase.
  bool ReuseIds;           ///<Id of particles excluded values ​​are reused.
  ullong TotalNp;          ///<Total number of simulated particles (no cuenta las particulas inlet no validas).
  unsigned IdMax;          ///<It is the maximum Id used.

  //-Monitors dt value.
  unsigned DtModif;       ///<Number of modifications on  dt computed when it is too low. | Numero de modificaciones del dt calculado por ser demasiado bajo.         
  unsigned DtModifWrn;    ///<Limit number for warning generation.
  double PartDtMin;       ///<Minimum value of dt in the current PART. | Valor minimo de dt en el PART actual.
  double PartDtMax;       ///<Maximum value of dt in the current PART. | Valor maximo de dt en el PART actual.
  
  //-Variables for simulation of PARTs.
  int PartIni;            ///<First generated PART.  | Primer PART generado. 
  int Part;               ///<Saves subsequent PART. | Siguiente PART a guardar.                                          
  int Nstep;              ///<Number of step in execution.             | Numero de paso en ejecucion.
  int PartNstep;          ///<Number of step when last PART was saved. | Numero de paso en el que se guardo el ultimo PART.
  unsigned PartOut;       ///<Total number of excluded particles. | Numero total de particulas excluidas al grabar el ultimo PART.
  double TimeStepIni;     ///<Initial instant of the simulation. | Instante inicial de la simulación.
  double TimeStep;        ///<Current instant of the simulation. | Instante actual de la simulación.                                 
  double TimeStepM1;      ///<Instant of the simulation when the last PART was stored. | Instante de la simulación en que se grabo el último PART.         
  double TimePartNext;    ///<Instant to store next PART file.   | Instante para grabar siguiente fichero PART.
  double LastDt;          ///<Last dt value added to Instant to TimeStep. | Ultimo valor de dt sumado a TimeStep.

  //-Control of the execution times.
  JTimer TimerTot;         ///<Measueres total runtime.                          | Mide el tiempo total de ejecucion.
  JTimer TimerSim;         ///<Measueres runtime since first step of simulation. | Mide el tiempo de ejecucion desde el primer paso de calculo.
  JTimer TimerPart;        ///<Measueres runtime since last PART.                | Mide el tiempo de ejecucion desde el ultimo PART.
  
  //-Execution variables.
  JPartsLoad4 *PartsLoaded;
  TpInterStep InterStep;
  int VerletStep;
  double SymplecticDtPre;  ///<Previous Dt to use with Symplectic.
  double DemDtForce;       ///<Dt for tangencial acceleration.
  StMaxNumbers MaxNumbers; ///<Maximum values (or almost) achieved during the simulation.


  bool SaveFtAce;    ///<Indicates whether linear and angular accelerations of each floating objects are saved.
  void SaveFtAceFun(double dt,bool predictor,StFtoForces *ftoforces);


  void AllocMemoryFloating(unsigned ftcount,bool imposedvel=false);
  llong GetAllocMemoryCpu()const;

  void LoadConfig(const JCfgRun *cfg);
  void LoadConfigCtes(const JXml *xml);
  void LoadConfigVars(const JXml *xml);
  void LoadConfigVarsExec();
  void LoadConfigParameters(const JXml *xml);
  void LoadConfigCommands(const JCfgRun *cfg);
  void LoadCaseConfig(const JCfgRun *cfg);

  StDemData LoadDemData(bool checkdata,const JSpacePartBlock* block)const;
  void VisuDemCoefficients()const;

  void LoadCodeParticles(unsigned np,const unsigned *idp,typecode *code)const;
  void LoadBoundNormals(unsigned np,unsigned npb,const unsigned *idp,const typecode *code,tfloat3 *boundnormal);  //<vs_mddbc>
  void ConfigBoundNormals(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp,tfloat3 *boundnormal); //<vs_mddbc>

  void PrepareCfgDomainValues(tdouble3 &v,tdouble3 vdef=TDouble3(0))const;
  void ResizeMapLimits();

  static void WendlandConstants(bool simulate2d,double h,float &awen,float &bwen);
  void ConfigConstants(bool simulate2d);
  void VisuConfig()const;
  void VisuParticleSummary()const;
  void LoadDcellParticles(unsigned n,const typecode *code,const tdouble3 *pos,unsigned *dcell)const;
  void RunInitialize(unsigned np,unsigned npb,const tdouble3 *pos,const unsigned *idp
    ,const typecode *code,tfloat4 *velrhop,tfloat3 *boundnormal);
  void CreatePartsInit(unsigned np,const tdouble3 *pos,const typecode *code);
  void FreePartsInit();

  void ConfigCellDivision();
  void SelecDomain(tuint3 celini,tuint3 celfin);
  static unsigned CalcCellCode(tuint3 ncells);
  void CalcFloatingRadius(unsigned np,const tdouble3 *pos,const unsigned *idp);
  tdouble3 UpdatePeriodicPos(tdouble3 ps)const;

  void RestartCheckData();
  void LoadCaseParticles();
  void InitRun(unsigned np,const unsigned *idp,const tdouble3 *pos);
  bool CalcMotion(double stepdt);
  void CalcMotionWaveGen(double stepdt);

  void PrintSizeNp(unsigned np,llong size,unsigned allocs)const;
  void PrintHeadPart();

  void ConfigSaveData(unsigned piece,unsigned pieces,std::string div);
  void AddParticlesOut(unsigned nout,const unsigned *idp,const tdouble3 *pos,const tfloat3 *vel,const float *rhop,const typecode *code);
  void AbortBoundOut(JLog2 *log,unsigned nout,const unsigned *idp,const tdouble3 *pos,const tfloat3 *vel,const float *rhop,const typecode *code);

  tfloat3* GetPointerDataFloat3(unsigned n,const tdouble3* v)const;
  void AddBasicArrays(JDataArrays &arrays,unsigned np,const tdouble3 *pos
    ,const unsigned *idp,const tfloat3 *vel,const float *rhop)const;
  void SavePartData(unsigned npok,unsigned nout,const JDataArrays& arrays,unsigned ndom,const tdouble3 *vdom,const StInfoPartPlus *infoplus);
  void SaveData(unsigned npok,const JDataArrays& arrays,unsigned ndom,const tdouble3 *vdom,const StInfoPartPlus *infoplus);
  void SaveDomainVtk(unsigned ndom,const tdouble3 *vdom)const;
  void SaveInitialDomainVtk()const;
  unsigned SaveMapCellsVtkSize()const;
  void SaveMapCellsVtk(float scell)const;
  void SaveVtkNormals(std::string filename,int numfile,unsigned np,unsigned npb                  //<vs_mddbc>
    ,const tdouble3 *pos,const unsigned *idp,const tfloat3 *boundnormal)const;                    //<vs_mddbc>

 
  void GetResInfo(float tsim,float ttot,const std::string &headplus,const std::string &detplus,std::string &hinfo,std::string &dinfo);
  void SaveRes(float tsim,float ttot,const std::string &headplus="",const std::string &detplus="");
  void ShowResume(bool stop,float tsim,float ttot,bool all,std::string infoplus);

  unsigned GetOutPosCount()const{ return(OutPosCount); }
  unsigned GetOutRhopCount()const{ return(OutRhopCount); }
  unsigned GetOutMoveCount()const{ return(OutMoveCount); }

public:
  JSph(bool cpu,bool mgpu,bool withmpi);
  ~JSph();

  static std::string GetStepName(TpStep tstep);
  static std::string GetKernelName(TpKernel tkernel);
  static std::string GetViscoName(TpVisco tvisco);
  static std::string GetBoundName(TpBoundary tboundary);
  static std::string GetSlipName(TpSlipMode tslip);       //<vs_mddbc>
  static std::string GetDDTName(TpDensity tdensity);

  std::string GetDDTConfig()const;

  static std::string TimerToText(const std::string &name,float value);

//-Functions for debug.
//----------------------
public:
  unsigned DgNum;
  void DgSaveVtkParticlesCpu(std::string filename,int numfile,unsigned pini,unsigned pfin,const tdouble3 *pos,const typecode *code,const unsigned *idp,const tfloat4 *velrhop,const tfloat3 *ace=NULL)const;
  void DgSaveVtkParticlesCpu(std::string filename,int numfile,unsigned pini,unsigned pfin,const tfloat3 *pos,const byte *check,const unsigned *idp,const tfloat3 *vel,const float *rhop);
  void DgSaveCsvParticlesCpu(std::string filename,int numfile,unsigned pini,unsigned pfin,std::string head,const tfloat3 *pos,const unsigned *idp=NULL,const tfloat3 *vel=NULL,const float *rhop=NULL,const float *ar=NULL,const tfloat3 *ace=NULL,const tfloat3 *vcorr=NULL);
};

/*:
ES:
Consideraciones sobre condiciones periodicas:
- Para cada eje periodico se define un valor tfloat3 para sumar a las particulas
  que se salgan por el extremo superior del dominio.
- En MapPosMin/Max se el anhade una holgura de H*BORDER_MAP, pero en el caso de
  condiciones periodicas esta holgura solo se aplica a MapPosMax.
- El ajuste de tamanho de dominio realizado por ResizeMapLimits() no afecta a los
  ejes periodicos.
- El halo periodico tendrá una unica celda de grosor 2h aunque en los otros ejes
  se use celdas de tamanho h.
- En la interaccion, una celda de tamanho 2h o dos celdas de tamanho h del extremo 
  inferior interaccionan con el halo periodico. En el caso del extremo superior
  deben ser 2 celdas de 2h o 3 celdas de h.
EN:
Considerations for periodic conditions:
- For each periodic edge a tfloat3 value is defined to be added to the particles
   that they get out at the limits of the domain.
- In MapPosMin/Max there is the added space of H*BORDER_MAP, but in the case of
   periodic conditions this space only applies to MapPosMax.
- The adjustment of the domain size by ResizeMapLimits() does not affect the
   periodic edges.
- The periodic halo will have a single cell thick 2h although in the other axes
   h cell size is used.
- In the interaction, a cell of size 2h or two cells of size h in the
   lower end interact with the periodic halo. For the upper limit
   there must be either 2 2h cells or 3 h cells.
:*/

#endif


