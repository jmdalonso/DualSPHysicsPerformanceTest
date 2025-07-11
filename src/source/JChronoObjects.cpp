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

/// \file JChronoObjects.cpp \brief Implements the class \ref JChronoObjects.

#include "JChronoObjects.h"
#include "DSPHChronoLib.h"
#include "JChronoData.h"
#include "Functions.h"
#include "FunctionsGeo3d.h"
#include "JLog2.h"
#include "JXml.h"
#include "JSpaceParts.h"
#include "JAppInfo.h"
#include "JSaveCsv2.h"
#include "JVtkLib.h"
#include "JRangeFilter.h"
#include "JSpaceVtkOut.h"
#include "JSphMk.h"
#include <cstring>
#include <cfloat>
#include <climits>
#include <algorithm>

using namespace std;

#ifndef DISABLE_CHRONO

//##############################################################################
//# JChronoObjects
//##############################################################################
//==============================================================================
/// Constructor.
//==============================================================================
JChronoObjects::JChronoObjects(JLog2* log,const std::string &dirdata,const std::string &casename
 ,const JXml *sxml,const std::string &place,double dp,word mkboundfirst)
 :Log(log),DirData(dirdata),CaseName(casename),Dp(dp),MkBoundFirst(mkboundfirst),UseDVI(true)
{
  ClassName="JChronoObjects";
  ChronoDataXml=NULL;
  ChronoLib=NULL;
  Ptr_VtkSimple_AutoActual=NULL;
  Ptr_VtkSimple_AutoDp=NULL;
  Reset();
  LoadXml(sxml,place);
}

//==============================================================================
/// Destructor.
//==============================================================================
JChronoObjects::~JChronoObjects(){
  DestructorActive=true;
  Reset();
}

//==============================================================================
/// Initialisation of variables.
//==============================================================================
void JChronoObjects::Reset(){
  Solver=0;
  OmpThreads=1;
  UseChronoSMC=false;
  UseCollision=false;
  WithMotion=false;
  delete ChronoDataXml; ChronoDataXml=NULL;
  delete ChronoLib; ChronoLib=NULL;
  CollisionDp=0;
  SchemeScale=1;
  SaveDataTime=NextTime=0;
  LastTimeOk=-1;
  JVtkLib::DeleteMkShapes(Ptr_VtkSimple_AutoActual); Ptr_VtkSimple_AutoActual=NULL;
  JVtkLib::DeleteMkShapes(Ptr_VtkSimple_AutoDp);     Ptr_VtkSimple_AutoDp=NULL;
}

//==============================================================================
/// Returns TRUE when a chrono object with geometry is defined for indicated mkbound.
//==============================================================================
bool JChronoObjects::UseDataDVI(word mkbound)const{
  const unsigned ipos=ChronoDataXml->BodyIndexByMkBound(mkbound);
  return(ipos<ChronoDataXml->GetBodyCount() && !ChronoDataXml->GetBody(ipos)->GetModelFile().empty());
}

//==============================================================================
/// Configures body with data from floating body. 
/// Returns TRUE when it is a floating with Chrono.
//==============================================================================
bool JChronoObjects::ConfigBodyFloating(word mkbound,double mass
  ,const tdouble3 &center,const tmatrix3d &inertia
  ,const tint3 &translationfree,const tint3 &rotationfree
  ,const tfloat3 &linvelini,const tfloat3 &angvelini)
{
  JChBodyFloating* body=(JChBodyFloating*)ChronoDataXml->GetBodyFloating(mkbound);
  if(body)body->SetFloatingData(mass,center,inertia,translationfree,rotationfree,linvelini,angvelini);
  return(body!=NULL);
}

//==============================================================================
/// Configures Floating body data for collisions. 
//==============================================================================
void JChronoObjects::ConfigDataBodyFloating(word mkbound,float kfric,float restitu,float young,float poisson){
  JChBodyFloating* body=(JChBodyFloating*)ChronoDataXml->GetBodyFloating(mkbound);
  if(body)body->SetCollisionData(kfric,restitu,young,poisson);
}
//==============================================================================
/// Configures Moving body data for collisions. 
//==============================================================================
void JChronoObjects::ConfigDataBodyMoving(word mkbound,float kfric,float restitu,float young,float poisson){
  JChBodyMoving* body=(JChBodyMoving*)ChronoDataXml->GetBodyMoving(mkbound);
  if(body)body->SetCollisionData(kfric,restitu,young,poisson);
}

//==============================================================================
/// Configures Fixed body data for collisions. 
//==============================================================================
void JChronoObjects::ConfigDataBodyFixed(word mkbound,float kfric,float restitu,float young,float poisson){
  JChBodyFixed* body=(JChBodyFixed*)ChronoDataXml->GetBodyFixed(mkbound);
  if(body)body->SetCollisionData(kfric,restitu,young,poisson);
}

//==============================================================================
/// Loads data from XML file.
//==============================================================================
void JChronoObjects::LoadXml(const JXml *sxml,const std::string &place){
  TiXmlNode* node=sxml->GetNodeSimple(place);
  if(!node)Run_Exceptioon(std::string("Cannot find the element \'")+place+"\'.");
  if(sxml->CheckNodeActive(node))ReadXml(sxml,node->ToElement());
}

//==============================================================================
/// Reads list of bodies and links in the XML node.
//==============================================================================
JChBody::TpModelNormal ReadXmlModelNormal(const JXml *sxml,TiXmlElement* ele){
  string mnormal=sxml->GetAttributeStr(ele,"modelnormal",true,"original");
  JChBody::TpModelNormal tnormal;
  if(fun::StrLower(mnormal)=="original")tnormal=JChBody::NorOriginal;
  else if(fun::StrLower(mnormal)=="invert")tnormal=JChBody::NorInvert;
  else if(fun::StrLower(mnormal)=="twoface")tnormal=JChBody::NorTwoFace;
  else sxml->ErrReadAtrib(ele,"modelnormal",false,"The value is invalid");
  return(tnormal);
}

//==============================================================================
/// Reads list of bodies and links in the XML node.
//==============================================================================
std::string JChronoObjects::ReadXmlModelFile(const JXml *sxml,TiXmlElement* ele)const{
  string mfile=sxml->GetAttributeStr(ele,"modelfile",true);
  if(!mfile.empty()){
    mfile=fun::StrReplace(mfile,"[CaseName]",CaseName);
    mfile=fun::StrReplace(mfile,"[casename]",CaseName);
  }
  return(mfile);
}

//==============================================================================
/// Loads pointer model data for AutoActual configuration.
//==============================================================================
void JChronoObjects::LoadPtrAutoActual(const JXml *sxml,std::string xmlrow){
  if(!JVtkLib::Available())Run_Exceptioon("Code for VTK format files is not included in the current compilation, so CHRONO collisions according to VTK geometry are not supported.");
  if(Ptr_VtkSimple_AutoActual==NULL){
    JSpaceVtkOut vtkout;
    vtkout.LoadXml(sxml,"case.execution.vtkout",false);
    std::vector<std::string> vtkfiles;
    vtkout.GetFiles("_Actual.vtk",vtkfiles);
    if(vtkfiles.size()==0)Run_ExceptioonFile("Actual VTK files not found for configuration AutoActual",xmlrow);
    for(unsigned c=0;c<unsigned(vtkfiles.size());c++)vtkfiles[c]=DirData+vtkfiles[c];
    Ptr_VtkSimple_AutoActual=JVtkLib::CreateMkShapes(vtkfiles);
  }
}

//==============================================================================
/// Loads pointer model data for AutoDp configuration.
//==============================================================================
void JChronoObjects::LoadPtrAutoDp(const JXml *sxml,std::string xmlrow){
  if(!JVtkLib::Available())Run_Exceptioon("Code for VTK format files is not included in the current compilation, so CHRONO collisions according to VTK geometry are not supported.");
  if(Ptr_VtkSimple_AutoDp==NULL){
    JSpaceVtkOut vtkout;
    vtkout.LoadXml(sxml,"case.execution.vtkout",false);
    std::vector<std::string> vtkfiles;
    vtkout.GetFiles("_Dp.vtk",vtkfiles);
    if(vtkfiles.size()==0)Run_ExceptioonFile("Dp VTK files not found for configuration AutoDp",xmlrow);
    for(unsigned c=0;c<unsigned(vtkfiles.size());c++)vtkfiles[c]=DirData+vtkfiles[c];
    Ptr_VtkSimple_AutoDp=JVtkLib::CreateMkShapes(vtkfiles);
  }
}

//==============================================================================
/// Creates and return obj file with body geometry.
//==============================================================================
void JChronoObjects::CreateObjFiles(std::string idname,const std::vector<unsigned> &mkbounds
  ,std::string datadir,std::string mfile,byte normalmode,std::string diroutobj,std::string xmlrow)
{
  const unsigned nmkbounds=unsigned(mkbounds.size());
  JVtkLib::TpModeNormal tnormal=JVtkLib::NorNULL;
  switch((JChBody::TpModelNormal)normalmode){
    case JChBody::NorOriginal: tnormal=JVtkLib::NorOriginal; break;
    case JChBody::NorInvert:   tnormal=JVtkLib::NorInvert;   break;
    case JChBody::NorTwoFace:  tnormal=JVtkLib::NorTwoFace;  break;
    default: Run_ExceptioonFile("Normal mode is unknown.",xmlrow);
  }
  const string mfext=fun::StrLower(fun::GetExtension(mfile));
  const bool auto_actual=fun::StrUpper(mfile)=="AUTOACTUAL";
  const bool auto_dp=fun::StrUpper(mfile)=="AUTODP";
  if(mfext=="obj"){
    if(nmkbounds>1)Run_ExceptioonFile("Model file OBJ is invalid for several MK values.",xmlrow);
    if(tnormal!=JVtkLib::NorOriginal)Run_ExceptioonFile("Only ModelNormal=Original is valid for Model file OBJ.",xmlrow);
    const string filein=datadir+mfile;
    const string fileout=diroutobj+idname+fun::PrintStr("_mkb%04u.obj",word(mkbounds[0]));
    //Log->Printf("-----> filein:[%s] -> fout[%s]",filein.c_str(),fileout.c_str());
    if(!fun::FileExists(filein))Run_ExceptioonFile("Error: File was not found.",filein);
    if(fun::CpyFile(filein,fileout))Run_ExceptioonFile("Error: File could not be created.",fileout);
  }
  else if(mfext=="vtk" || auto_actual || auto_dp){
    void* ptr=NULL;
    if(auto_actual){
      if(Ptr_VtkSimple_AutoActual==NULL)Run_Exceptioon("Error: Ptr_VtkSimple_AutoActual is not defined.");
      ptr=Ptr_VtkSimple_AutoActual;
    }
    if(auto_dp){
      if(Ptr_VtkSimple_AutoDp==NULL)Run_Exceptioon("Error: Ptr_VtkSimple_AutoDp is not defined.");
      ptr=Ptr_VtkSimple_AutoDp;
    }
    const string filein=(ptr==NULL? datadir+mfile: "");
    JVtkLib::CreateOBJsByMk(ptr,filein,diroutobj+idname,mkbounds,MkBoundFirst,tnormal);
  }
  else Run_ExceptioonFile("Model file format is invalid.",xmlrow);
}

//==============================================================================
/// Reads list of bodies and links in the XML node.
//==============================================================================
void JChronoObjects::ReadXml(const JXml *sxml,TiXmlElement* lis){
  Reset();
  //-Checks XML elements.
  sxml->CheckElementNames(lis,true,"savedata schemescale collision *bodyfixed *bodyfloating *link_hinge *link_spheric *link_pointline *link_linearspring *link_coulombdamping *link_pulley");

  ChronoDataXml=new JChronoData;
  ChronoDataXml->SetUseNSCChrono(UseDVI);
  ChronoDataXml->SetDp(Dp);
  //-Create dir for OBJ files with geometry.
  const string diroutobj=AppInfo.GetDirOut()+"chrono_objs/";
  fun::MkdirPath(diroutobj);
  Log->AddFileInfo(diroutobj+"XXXX_mkXXXX.obj","Geometry files used for Chrono interaction.");
  ChronoDataXml->SetDataDir(diroutobj);
  //-Loads configuration to save CSV file for debug.
  SaveDataTime=sxml->ReadElementFloat(lis,"savedata","value",true,-1.f);
  //-Loads scale value to create initial scheme of configuration.
  SchemeScale=sxml->ReadElementFloat(lis,"schemescale","value",true,1);

  //-Configures the collision for chrono.
  if(sxml->ExistsElement(lis,"collision")){
    TiXmlElement *collision=lis->FirstChildElement("collision");
    UseCollision=sxml->GetAttributeBool(collision,"activate",false);
    if(UseCollision){
      sxml->CheckElementNames(collision,true,"distancedp ompthreads contactmethod");
      //-Loads allowed collision overlap according Dp.
      CollisionDp=sxml->ReadElementFloat(collision,"distancedp","value",true,0.5f);
      ChronoDataXml->SetCollisionDp(CollisionDp);
      //-Loads solver. Only DSolverType::BB is allowed yet
      const unsigned solver=JChronoData::DSolverType::BB;//sxml->ReadElementUnsigned(collision,"solver","value",true, JChronoData::DSolverType::BB);
      //const unsigned iteration_max=sxml->ReadElementUnsigned(collision,"solver","iteration_max",true,10);
      if(solver!=JChronoData::DSolverType::APGD && solver!=JChronoData::DSolverType::APGDREF && solver!=JChronoData::DSolverType::BB)Run_ExceptioonFile(fun::PrintStr("The solver value \'%d\' is not allowed.",solver),sxml->ErrGetFileRow(lis));
      ChronoDataXml->SetSolver(solver);
      //ChronoDataXml->SetMaxIter(iteration_max);
      //-Loads number of threads.
      OmpThreads=sxml->ReadElementInt(collision,"ompthreads","value",true,1); //-Default=Single-core
      //-Loads the contact method type [NSC|SMC]
      unsigned contact_m=sxml->ReadElementInt(collision,"contactmethod","value",true,0);
      if(contact_m<0 || contact_m>1)Run_ExceptioonFile(fun::PrintStr("The value \'%d\' is not allowed for contactmethod attribute. Only 0 or 1.",contact_m),sxml->ErrGetFileRow(lis)); 
      ChronoDataXml->SetContactMethod(contact_m==0? JChronoData::NSC: JChronoData::SMC);
      UseChronoSMC=(ChronoDataXml->GetContactMethod()==JChronoData::SMC);
    }
  }
  ConfigOmp();
  ChronoDataXml->SetOmpThreads(OmpThreads);

  //-Loads body elements.
  unsigned nextidbody=0;
  TiXmlElement* ele=lis->FirstChildElement(); 
  while(ele){
    const std::string elename=ele->Value();
    if(elename.length()>4 && elename.substr(0,4)=="body" && sxml->CheckElementActive(ele)){
      const string xmlrow=sxml->ErrGetFileRow(ele);
      string idnamebase=sxml->GetAttributeStr(ele,"id");
      //word mkbound=sxml->GetAttributeWord(ele,"mkbound");
      std::vector<unsigned> mkbounds;
      JRangeFilter rg(sxml->GetAttributeStr(ele,"mkbound"));
      //Log->Printf("-----> mkbounds:[%s]",rg.ToString().c_str());
      rg.GetValues(mkbounds);
      const unsigned nmkbounds=unsigned(mkbounds.size());

      //-Creates obj files with geometry if collisions are activated.
      std::string mfilebase="";
      JChBody::TpModelNormal tnormal=JChBody::NorOriginal;
      if(UseCollision){
        mfilebase=ReadXmlModelFile(sxml,ele);
        if(fun::StrUpper(mfilebase)=="AUTOACTUAL" && Ptr_VtkSimple_AutoActual==NULL)LoadPtrAutoActual(sxml,xmlrow);
        if(fun::StrUpper(mfilebase)=="AUTODP"     && Ptr_VtkSimple_AutoDp    ==NULL)LoadPtrAutoDp    (sxml,xmlrow);
        tnormal=ReadXmlModelNormal(sxml,ele);
        if(!mfilebase.empty())CreateObjFiles(idnamebase,mkbounds,DirData,mfilebase,byte(tnormal),diroutobj,xmlrow);
      }
      //-Creates a body object for each MK value in mkbounds[].
      for(unsigned cmk=0;cmk<nmkbounds;cmk++){
        const word mkbound=word(mkbounds[cmk]);
        //-Creates body object.
        unsigned idb=nextidbody; nextidbody++;
        const string idname=(nmkbounds>1? idnamebase+fun::UintStr(mkbound): idnamebase);
        const string mfile=(!mfilebase.empty()? idnamebase+fun::PrintStr("_mkb%04u.obj",mkbound): "");
        if(elename=="bodyfloating"){
          //<vs_chroonodev_ini>
          //Log->Printf("----> AddBodyFloating>> \'%s\' mkb:%u mf:[%s]",idname.c_str(),mkbound,mfile.c_str());
          //const unsigned type=sxml->GetAttributeUnsigned(ele,"type",true,0);
          //JChBodyFloating *body=NULL;
          //if(type==0) body=ChronoDataXml->AddBodyFloating(idb,idname,mkbound,xmlrow); //-No Finite Element
          //else        body=ReadElementFEA(sxml,ele,idb,idname,mkbound);               //-For Finite Elements
          //<vs_chroonodev_end>
          JChBodyFloating *body=ChronoDataXml->AddBodyFloating(idb,idname,mkbound,xmlrow);
          if(UseCollision)body->SetModel(mfile,tnormal);
          ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
        }
        else if(elename=="bodymoving"){
          Run_Exceptioon("The use of predefined moving objects (<bodymoving>) is disabled since it does not work properly.");
          //Log->Printf("----> AddBodyMoving>> \'%s\' mkb:%u mf:[%s]",idname.c_str(),mkbound,mfile.c_str());
          const double mass=sxml->GetAttributeDouble(ele,"massbody");
          JChBodyMoving *body=ChronoDataXml->AddBodyMoving(idb,idname,mkbound,mass,xmlrow);
          if(UseCollision)body->SetModel(mfile,tnormal);
          ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
          if(!mfile.empty())WithMotion=true;
        }
        else if(elename=="bodyfixed"){
          //Log->Printf("----> AddBodyFixed>> \'%s\' mkb:%u mf:[%s]",idname.c_str(),mkbound,mfile.c_str());
          JChBodyFixed *body=ChronoDataXml->AddBodyFixed(idb,idname,mkbound,xmlrow);
          if(UseCollision)body->SetModel(mfile,tnormal);
          ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
        }
        else sxml->ErrReadElement(ele,elename,false);
      }
    }
    ele=ele->NextSiblingElement();
  }

  //-Loads link elements.
  ele=lis->FirstChildElement(); 
  while(ele){
    const std::string elename=ele->Value();
    if(elename.length()>5 && elename.substr(0,5)=="link_" && sxml->CheckElementActive(ele)){
      const string xmlrow=sxml->ErrGetFileRow(ele);
      //-Identify body1.
      const string idnamebody1=sxml->GetAttributeStr(ele,"idbody1");
      unsigned idx1=ChronoDataXml->BodyIndexByName(idnamebody1);
      if(idx1==UINT_MAX)Run_ExceptioonFile(fun::PrintStr("idbody1 \'%s\' is not found.",idnamebody1.c_str()),xmlrow);
      const unsigned idbody1=ChronoDataXml->GetBody(idx1)->Idb;
      //-Identify body2.
      const string idnamebody2=sxml->GetAttributeStr(ele,"idbody2",true,"NULL");
      unsigned idbody2=UINT_MAX;
      if(idnamebody2!="NULL"){
        unsigned idx2=ChronoDataXml->BodyIndexByName(idnamebody2);
        if(idx2==UINT_MAX)Run_ExceptioonFile(fun::PrintStr("idbody2 \'%s\' is not found.",idnamebody2.c_str()),xmlrow);
        idbody2=ChronoDataXml->GetBody(idx2)->Idb;
      }
      //-Defines link name.
      string name=sxml->GetAttributeStr(ele,"name",true);
      if(name.empty()){
        if(idbody2!=UINT_MAX)name=fun::PrintStr("Link_%s_%s",idnamebody1.c_str(),idnamebody2.c_str());
        else name=fun::PrintStr("Link_%s",idnamebody1.c_str());
        if(ChronoDataXml->LinkIndexByName(name)!=UINT_MAX){//-Adds number to name when the name already exists.
          int num=2;
          string name2=name+"_"+fun::IntStr(num);
          while(ChronoDataXml->LinkIndexByName(name2)!=UINT_MAX){
            num++;
            name2=name+"_"+fun::IntStr(num);
          }
          name=name2;
        }
      }
      //-Creates link object.
      if(elename=="link_hinge"){
        JChLinkHinge *link=ChronoDataXml->AddLinkHinge(name,idbody1,idbody2,xmlrow);
        link->SetRotPoint (sxml->ReadElementDouble3(ele,"rotpoint" ));
        link->SetRotVector(sxml->ReadElementDouble3(ele,"rotvector"));
        link->SetStiffness(sxml->ReadElementDouble (ele,"stiffness","value"));
        link->SetDamping  (sxml->ReadElementDouble (ele,"damping"  ,"value"));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
      }
      else if(elename=="link_spheric"){
        JChLinkSpheric *link=ChronoDataXml->AddLinkSpheric(name,idbody1,idbody2,xmlrow);
        link->SetRotPoint (sxml->ReadElementDouble3(ele,"rotpoint"));
        link->SetStiffness(sxml->ReadElementDouble (ele,"stiffness","value"));
        link->SetDamping  (sxml->ReadElementDouble (ele,"damping"  ,"value"));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
      }
      else if(elename=="link_pointline"){
        if(idnamebody2!="NULL")Run_ExceptioonFile("Link-PointLine only uses one body.",xmlrow);
        JChLinkPointLine *link=ChronoDataXml->AddLinkPointLine(name,idbody1,idbody2,xmlrow);
        link->SetSlidingVector(sxml->ReadElementDouble3(ele,"slidingvector"));
        link->SetRotPoint (sxml->ReadElementDouble3(ele,"rotpoint"));
        link->SetRotVector(sxml->ExistsElement(ele,"rotvector")? sxml->ReadElementDouble3(ele,"rotvector"): TDouble3(0));
        link->SetRotVector2(sxml->ExistsElement(ele,"rotvector") && sxml->ExistsElement(ele,"rotvector2")? sxml->ReadElementDouble3(ele,"rotvector2"): TDouble3(0));
        link->SetStiffness(sxml->ReadElementDouble (ele,"stiffness","value"));
        link->SetDamping  (sxml->ReadElementDouble (ele,"damping"  ,"value"));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
      }
      else if(elename=="link_linearspring"){
        JChLinkLinearSpring *link=ChronoDataXml->AddLinkLinearSpring(name,idbody1,idbody2,xmlrow);
        link->SetPointfb0  (sxml->ReadElementDouble3(ele,"point_fb1"));
        link->SetPointfb1  (sxml->ReadElementDouble3(ele,"point_fb2"));
        link->SetStiffness (sxml->ReadElementDouble (ele,"stiffness"  ,"value"));
        link->SetDamping   (sxml->ReadElementDouble (ele,"damping"    ,"value"));
        link->SetRestLength(sxml->ReadElementDouble (ele,"rest_length","value"));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
        TiXmlElement* ele2=ele->FirstChildElement("savevtk");
        if(ele2){//-Configuration to save vtk spring.
          JChLink::StSaveSpring cfg;
          cfg.radius=sxml->ReadElementFloat(ele2,"radius","value",true,cfg.radius);
          cfg.length=sxml->ReadElementFloat(ele2,"length","value",true,cfg.length);
          cfg.nside =sxml->ReadElementInt  (ele2,"nside" ,"value",true,cfg.nside );
          link->SetSvSpring(cfg);
        }
      }
      else if(elename=="link_coulombdamping"){
        JChLinkCoulombDamping *link=ChronoDataXml->AddLinkCoulombDamping(name,idbody1,idbody2,xmlrow);
        link->SetPointfb0  (sxml->ReadElementDouble3(ele,"point_fb1"));
        link->SetPointfb1  (sxml->ReadElementDouble3(ele,"point_fb2"));
        link->SetRestLength(sxml->ReadElementDouble (ele,"rest_length","value"));
        link->SetCoulombDamping(sxml->ReadElementDouble(ele,"damping","value",false));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
        TiXmlElement* ele2=ele->FirstChildElement("savevtk");
        if(ele2){//-Configuration to save vtk spring.
          JChLink::StSaveSpring cfg;
          cfg.radius=sxml->ReadElementFloat(ele2,"radius","value",true,cfg.radius);
          cfg.length=sxml->ReadElementFloat(ele2,"length","value",true,cfg.length);
          cfg.nside =sxml->ReadElementInt  (ele2,"nside" ,"value",true,cfg.nside );
          link->SetSvSpring(cfg);
        }
      }
      else if(elename=="link_pulley"){
        if(idnamebody1=="NULL"&&idnamebody2=="NULL")Run_ExceptioonFile("Link-Pulley uses two bodies.",xmlrow);
        if(!ChronoDataXml->BodyBelongsLink(idbody1,JChLink::LK_Hinge))Log->PrintfWarning("The body \"%s\" also should belong to a link_hinge.",idnamebody1.c_str());
        JChLinkPulley *link=ChronoDataXml->AddLinkPulley(name,idbody1,idbody2,xmlrow);
        link->SetRotPoint (sxml->ReadElementDouble3(ele,"rotpoint" ));
        link->SetRotVector(sxml->ReadElementDouble3(ele,"rotvector"));
        link->SetRadius(sxml->ReadElementFloat(ele,"radius","value"));
        link->SetRadius2(sxml->ReadElementFloat(ele,"radius2","value"));
        ReadXmlValues(sxml,ele->FirstChildElement("values"),link->GetValuesPtr());
      }
      else sxml->ErrReadElement(ele,elename,false);
    }
    ele=ele->NextSiblingElement();
  }
  //-Prepares object ChronoDataXml.
  ChronoDataXml->Prepare();
  NextTime=(SaveDataTime>=0? 0: DBL_MAX);
  LastTimeOk=-1;
}

//<vs_chroonodev_ini>
//==============================================================================
/// Reads the Finit Elements from the XML file
//==============================================================================
//JChBodyFloating* JChronoObjects::ReadElementFEA(const JXml *sxml,TiXmlElement* ele,unsigned idb,const std::string idname,const word mkbound) {
//  const string xmlrow=sxml->ErrGetFileRow(ele);
//  JChElementFEA* body_ret=NULL; //-Body to return
//  TiXmlElement ele_fea=*ele; 
//  //-Reading Finite Element Cable
//  if(sxml->ExistsElement(&ele_fea,"cable")){
//    JChCable *body=ChronoDataXml->AddCableFloating(idb,idname,mkbound,xmlrow);
//    ChronoDataXml->SetUseFEA(true); //This configures the rigth solver for Finite Elements 
//    body->SetUseFEA(true);
//    ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
//    //-Cable element
//    TiXmlElement *ele_cable=ele_fea.FirstChildElement("cable");
//    //-Cable properties
//    body->SetSegments(sxml->ReadElementUnsigned(ele_cable,"segments","value",true,1));
//    body->SetPointA(sxml->ReadElementFloat3(ele_cable,"pointA"));
//    body->SetPointB(sxml->ReadElementFloat3(ele_cable,"pointB"));
//    //-Section Cable properties
//    TiXmlElement *ele_section=ele_cable->FirstChildElement("section");
//    JChCable::StSection *section=body->GetSection();
//    section->Area=sxml->ReadElementDouble(ele_section,"area","value",true,0.0);
//    section->Diameter=sxml->ReadElementDouble(ele_section,"diameter","value",true,0.0);
//    section->Density=sxml->ReadElementDouble(ele_section,"density","value",true,1000);
//    section->RaleyghDamping=sxml->ReadElementDouble(ele_section,"raleyghdamping","value",true,0.0);
//    if(section->Area==0.0&&section->Diameter==0.0)Run_ExceptioonFile(fun::PrintStr("Diameter and area values cannot be zero to create the section of cable. Introduce a value for one of them."),sxml->ErrGetFileRow(ele_section)); 
//    body->SetSection(*section);
//    body_ret=body;
//  }
//  //-Reading Finite Element Beam Euler-Bernoulli
//  else if(sxml->ExistsElement(&ele_fea, "beamEuler")) {
//    JChBeamEuler *body=ChronoDataXml->AddBeamEulerFloating(idb,idname,mkbound,xmlrow);
//    ChronoDataXml->SetUseFEA(true); //This configures the rigth solver for Finite Elements 
//    body->SetUseFEA(true);
//    ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
//    //-Beam element
//    TiXmlElement *ele_cable=ele_fea.FirstChildElement("beamEuler");
//    //-Beam properties
//    body->SetSegments(sxml->ReadElementUnsigned(ele_cable,"segments","value",true,1));
//    body->SetPointA(sxml->ReadElementFloat3(ele_cable,"pointA","value"));
//    body->SetPointB(sxml->ReadElementFloat3(ele_cable,"pointB","value"));
//    body->SetDirection(sxml->ReadElementFloat3(ele_cable,"direction"));
//    //-Material Beam properties
//    TiXmlElement *ele_section=ele_cable->FirstChildElement("section");
//    JChBeamEuler::StSectionAdv *section=new JChBeamEuler::StSectionAdv();
//    section->Alpha=sxml->ReadElementDouble(ele_section,"alpha","value",true,0);
//    section->Width=sxml->ReadElementDouble(ele_section,"width","value");
//    section->Height=sxml->ReadElementDouble(ele_section,"heigth","value");
//    section->Density=sxml->ReadElementDouble(ele_section,"density","value",true,1000);
//    section->Stiffness=sxml->ReadElementDouble(ele_section,"stiffness","value",true,0.01e9);
//    section->RaleyghDamping=sxml->ReadElementDouble(ele_section,"raleyghdamping","value",true,0.0);
//    section->Centroid=sxml->ReadElementDouble3(ele_section,"centroid",true,TDouble3(0));
//    section->ShearCenter=sxml->ReadElementDouble3(ele_section,"shearCenter",true,TDouble3(0));
//    body->SetSection(*section);
//    body_ret=body;
//  }
//  //-Reading Finite Element Beam ANCF
//  else if (sxml->ExistsElement(&ele_fea, "beamANCF")) {
//    JChBeamANCF *body=ChronoDataXml->AddBeamANCFFloating(idb,idname,mkbound,xmlrow);
//    ChronoDataXml->SetUseFEA(true); //This configures the rigth solver for Finite Elements 
//    body->SetUseFEA(true);
//    ReadXmlValues(sxml,ele->FirstChildElement("values"),body->GetValuesPtr());
//    //-Beam element
//    TiXmlElement *ele_cable=ele_fea.FirstChildElement("beamANCF");
//    //-Beam properties
//    body->SetSegments(sxml->ReadElementUnsigned(ele_cable,"segments","value",true,1));
//    body->SetPointA(sxml->ReadElementFloat3(ele_cable,"pointA","value"));
//    body->SetPointB(sxml->ReadElementFloat3(ele_cable,"pointB","value"));
//    body->SetHeight(sxml->ReadElementDouble(ele_cable,"heigth","value"));
//    body->SetWidth(sxml->ReadElementDouble(ele_cable,"width","value"));
//    body->SetDamping(sxml->ReadElementDouble(ele_cable,"damping","value"));
//    body->SetUseGravity(sxml->ReadElementUnsigned(ele_cable,"useGravity","value")==0?false:true);
//    body->SetCurvature(sxml->ReadElementFloat3(ele_cable,"curvature"));
//    body->SetDirection(sxml->ReadElementFloat3(ele_cable,"direction"));
//    //-Material Beam properties
//    TiXmlElement *ele_section=ele_cable->FirstChildElement("material");
//    JChBeamANCF::StMaterial *material=new JChBeamANCF::StMaterial();
//    material->Poisson=sxml->ReadElementDouble(ele_section,"poisson","value");
//    material->Young=sxml->ReadElementDouble(ele_section,"young","value");
//    material->Stiffness=sxml->ReadElementDouble(ele_section,"young","value");
//    material->Density=sxml->ReadElementDouble(ele_section,"density","value",true,1000);
//    material->K1=sxml->ReadElementDouble(ele_section,"k1","value",true,0.0);
//    material->K2=sxml->ReadElementDouble(ele_section,"k2","value",true,0.0);
//    body->SetMaterial(*material);
//    //-Set the use of PoissonRatio
//    body->SetUsePoisson(material->Poisson==0?false:true);
//    body_ret=body;
//  }else Run_ExceptioonFile(fun::PrintStr("The type of Finite Element introduced is not allowed."),sxml->ErrGetFileRow(ele)); 
//
//  return(body_ret);
//}
//<vs_chroonodev_end>

//==============================================================================
/// Reads list of values in the XML node.
//==============================================================================
void JChronoObjects::ReadXmlValues(const JXml *sxml,TiXmlElement* lis,JChValues* values){
  if(lis){
    //-Loads elements bodyfloating.
    TiXmlElement* ele=lis->FirstChildElement(); 
    while(ele){
      const string cmd=ele->Value();
      if(cmd.length()&&cmd[0]!='_'){
        const bool triple=(sxml->ExistsAttribute(ele,"x") && sxml->ExistsAttribute(ele,"y") && sxml->ExistsAttribute(ele,"z"));
        if(cmd=="vstr")values->AddValueStr(sxml->GetAttributeStr(ele,"name"),sxml->GetAttributeStr(ele,"v"));
        else if(cmd=="vint"){
          const string name=sxml->GetAttributeStr(ele,"name");
          if(triple)values->AddValueInt3(name,sxml->GetAttributeInt3(ele));
          else      values->AddValueInt (name,sxml->GetAttributeInt (ele,"v"));
        }
        else if(cmd=="vuint"){
          const string name=sxml->GetAttributeStr(ele,"name");
          if(triple)values->AddValueUint3(name,sxml->GetAttributeUint3(ele));
          else      values->AddValueUint (name,sxml->GetAttributeUint (ele,"v"));
        }
        else if(cmd=="vreal"){
          const string name=sxml->GetAttributeStr(ele,"name");
          if(triple)values->AddValueDouble3(name,sxml->GetAttributeDouble3(ele));
          else      values->AddValueDouble (name,sxml->GetAttributeDouble (ele,"v"));
        }
        else sxml->ErrReadElement(ele,cmd,false);
      }
      ele=ele->NextSiblingElement();
    }
  }
}

//==============================================================================
/// Creates VTK file with the scheme of Chrono objects.
//==============================================================================
void JChronoObjects::SaveVtkScheme_Spring(JVtkLib *sh,word mk,word mk1
  ,tdouble3 pt0,tdouble3 pt1,double restlength,double radius,double revlength,int nside)const
{
  const double ds=Dp*SchemeScale;
  if(nside>1){
    const double cornersout=radius/2.,cornersin=radius/4.;
    sh->AddShapeSpring(pt0,pt1,restlength,ds,cornersout,cornersin,radius,revlength,nside,mk);
  }
  else sh->AddShapeLine(pt0,pt1,mk);
  sh->AddShapeBoxSize(pt0-TDouble3(ds),TDouble3(ds*2),mk);
  sh->AddShapeBoxSize(pt1-TDouble3(ds),TDouble3(ds*2),mk1);
}

//==============================================================================
/// Creates VTK file with the scheme of Chrono objects.
//==============================================================================
void JChronoObjects::SaveVtkScheme()const{
  const double ds=Dp*SchemeScale;
  JVtkLib sh;
  //-Represents floating objects.
  for(unsigned c=0;c<ChronoDataXml->GetBodyCount();c++){
    const JChBody* body=ChronoDataXml->GetBody(c);
    if(body->Type==JChBody::BD_Floating){
      const word mk=body->MkBound+MkBoundFirst;
      const tdouble3 center=body->GetCenter();
      sh.AddShapeCross(center,ds*2,mk);
    }
  }
  //-Represents links.
  for(unsigned c=0;c<ChronoDataXml->GetLinkCount();c++){
    const JChLink *link=ChronoDataXml->GetLink(c);
    if(link->GetBodyRefCount()<1)Run_Exceptioon("Link without body reference.");
    const word mk=link->GetBodyRef(0)->MkBound+MkBoundFirst;
    switch(link->Type){
      case JChLink::LK_Hinge:{
        const JChLinkHinge* linktype=(const JChLinkHinge*)link;
        const tdouble3 pt=linktype->GetRotPoint();
        const tdouble3 v=fgeo::VecUnitary(linktype->GetRotVector())*(ds*2);
        sh.AddShapeCylinder(pt-v,pt+v,ds*1.5,16,mk);
      }break;
      case JChLink::LK_Spheric:{
        const JChLinkSpheric* linktype=(const JChLinkSpheric*)link;
        sh.AddShapeSphere(linktype->GetRotPoint(),ds*2,16,mk);
      }break;
      case JChLink::LK_PointLine:{
        const JChLinkPointLine* linktype=(const JChLinkPointLine*)link;
        const tdouble3 pt=linktype->GetRotPoint();
        const tdouble3 v=fgeo::VecUnitary(linktype->GetSlidingVector())*(ds*4);
        const tdouble3 vrot=fgeo::VecUnitary(linktype->GetRotVector())*(ds*4);
        const tdouble3 vrot2=fgeo::VecUnitary(linktype->GetRotVector2())*(ds*4);
        if(vrot==TDouble3(0))sh.AddShapeSphere(linktype->GetRotPoint(),ds*2,16,mk);
        else{ 
          sh.AddShapeCylinder(pt-vrot,pt+vrot,ds*1.5,16,mk);
          if(vrot2!=TDouble3(0))sh.AddShapeCylinder(pt-vrot2,pt+vrot2,ds*1.5,16,mk);
        }
        sh.AddShapeLine(pt-(v*4.),pt+(v*4.),mk);
      }break;
      case JChLink::LK_LinearSpring:{
        const JChLinkLinearSpring* linktype=(const JChLinkLinearSpring*)link;
        if(link->GetBodyRefCount()<2)Run_Exceptioon("Link without two bodies reference.");
        const word mk1=link->GetBodyRef(1)->MkBound+MkBoundFirst;
        const JChLink::StSaveSpring cfg=linktype->GetSvSpring();
        SaveVtkScheme_Spring(&sh,mk,mk1,linktype->GetPointfb0(),linktype->GetPointfb1(),linktype->GetRestLength(),cfg.radius,cfg.length,cfg.nside);
      }break;
      case JChLink::LK_CoulombDamping:{
        const JChLinkCoulombDamping* linktype=(const JChLinkCoulombDamping*)link;
        if(link->GetBodyRefCount()<2)Run_Exceptioon("Link without two bodies reference.");
        const word mk1=link->GetBodyRef(1)->MkBound+MkBoundFirst;
        const JChLink::StSaveSpring cfg=linktype->GetSvSpring();
        SaveVtkScheme_Spring(&sh,mk,mk1,linktype->GetPointfb0(),linktype->GetPointfb1(),linktype->GetRestLength(),cfg.radius,cfg.length,cfg.nside);
      }break;
      case JChLink::LK_Pulley:{
        const JChLinkPulley* linktype=(const JChLinkPulley*)link;
        const tdouble3 pt=linktype->GetRotPoint();
        const tdouble3 v=fgeo::VecUnitary(linktype->GetRotVector())*(ds*2);
        sh.AddShapeCylinder(pt-v,pt+v,ds*1.5,16,mk);
      }break;
      default: Run_Exceptioon("Type of link is not supported.");
    }
  }
  const string filevtk=AppInfo.GetDirOut()+"CfgChrono_Scheme.vtk";
  Log->AddFileInfo(filevtk,"Saves VTK file with scheme of Chrono objects and links between objects.");
  sh.SaveShapeVtk(filevtk,"Mk");
}

//==============================================================================
/// Configures center of moving bodies starting from particles domains.
//==============================================================================
void JChronoObjects::ConfigMovingBodies(const JSphMk* mkinfo){
  for(unsigned c=0;c<ChronoDataXml->GetBodyCount();c++){
    const JChBody* body=ChronoDataXml->GetBody(c);
    if(body->Type==JChBody::BD_Moving){
      unsigned cb=mkinfo->GetMkBlockByMkBound(body->MkBound);
      if(cb>=mkinfo->Size())Run_Exceptioon(fun::PrintStr("Center of body \'%s\' (mkbound=%u) is not available.",body->IdName.c_str(),body->MkBound));
      const tdouble3 pcen=(mkinfo->Mkblock(cb)->GetPosMin()+mkinfo->Mkblock(cb)->GetPosMax())/2.;
      ((JChBodyMoving*)body)->SetInitialCenter(pcen);
    }
  }
}

//==============================================================================
/// Loads the execution configuration with OpenMP.
/// Carga la configuracion de ejecucion con OpenMP.
//==============================================================================
void JChronoObjects::ConfigOmp(){
#ifdef OMP_USE
  //-Determine number of threads for host with OpenMP. | Determina numero de threads por host con OpenMP.
  if (OmpThreads<=0)OmpThreads=max(omp_get_num_procs(), 1);
  if (OmpThreads>OMP_MAXTHREADS)OmpThreads=OMP_MAXTHREADS;
  omp_set_num_threads(OmpThreads);
  Log->Printf("Threads by host for parallel execution in Chrono: %d", OmpThreads);
#else
  OmpThreads=1;
#endif
}

//==============================================================================
/// Configures and reads floating data from XML file.
//==============================================================================
void JChronoObjects::Init(bool simulate2d,const JSphMk* mkinfo){
  //-Updates center of moving objects.
  ConfigMovingBodies(mkinfo);
  //-Checks data in ChronoData.
  ChronoDataXml->CheckData();
  //-Creates VTK file with the scheme of Chrono objects.
  SaveVtkScheme();
  //-Creates and configures object ChronoLib.
  if(ChronoDataXml->GetOmpThreads()>1){
    #ifndef DISABLE_CHRONO_OMP
      ChronoLib=new DSPHChronoLibMC(*ChronoDataXml); //<chrono_multicore>
    #else
      Log->PrintWarning("Chrono Parallel module is not enabled. The execution will be using single core");
      ChronoDataXml->SetOmpThreads(1);
      ChronoLib=new DSPHChronoLibSC(*ChronoDataXml);
    #endif
  }
  else ChronoLib=new DSPHChronoLibSC(*ChronoDataXml);
  const bool svforces=SaveDataTime>=0;
  ChronoLib->Config(AppInfo.GetDirOut(),svforces,simulate2d);
  if(svforces){
    Log->AddFileInfo("ChronoBody_forces.csv","Saves forces for each body.");
    Log->AddFileInfo("ChronoLink_forces.csv","Saves forces for each link.");
  }
  delete ChronoDataXml; ChronoDataXml=NULL;
  ChronoLib->Config_Inertia();
}

//==============================================================================
/// Check the parameters of the sent body to find missing values
//==============================================================================
void JChronoObjects::CheckParams(const JChBody *body)const{
  const string objdesc=fun::PrintStr("Object mkbound=%u",body->MkBound);
  if(body->GetKfric()  ==FLT_MAX)  Run_Exceptioon(objdesc+" - Value of Kfric is invalid.");
  if(body->GetRestitu()==FLT_MAX)  Run_Exceptioon(objdesc+" - Value of Restitution_Coefficient is invalid.");
  if(UseChronoSMC){
    if(body->GetYoung()  ==FLT_MAX)Run_Exceptioon(objdesc+" - Value of Young_Modulus is invalid.");
    if(body->GetPoisson()==FLT_MAX)Run_Exceptioon(objdesc+" - Value of PoissonRatio is invalid.");
  } 
}

//==============================================================================
/// Shows values configuration using Log.
//==============================================================================
void JChronoObjects::VisuValues(const JChValues *values)const{
  if(values->GetCount()){
    Log->Printf("    Values...: %u",values->GetCount());
    int lenmax=0;
    for(byte mode=0;mode<2;mode++){
      for(unsigned c=0;c<values->GetCount();c++){
        const JChValues::StValue* v=values->GetValue(c);
        string vtx;
        switch(v->type){
          case JChValues::TP_Text:     vtx=v->vtext;                      break;
          case JChValues::TP_Int:      vtx=fun::IntStr(v->vint);          break;
          case JChValues::TP_Uint:     vtx=fun::UintStr(v->vuint);        break;
          case JChValues::TP_Double:   vtx=fun::DoubleStr(v->vdouble);    break;
          case JChValues::TP_Int3:     vtx=fun::Int3Str(v->vint3);        break;
          case JChValues::TP_Uint3:    vtx=fun::Uint3Str(v->vuint3);      break;
          case JChValues::TP_Double3:  vtx=fun::Double3Str(v->vdouble3);  break;
          default: Run_Exceptioon("Type of value is invalid.");
        }
        if(mode==0){
          int len=(int)fun::PrintStr("      %s <%s>",v->name.c_str(),JChValues::TypeToStr(v->type).c_str()).size();
          lenmax=max(lenmax,len);
        }
        else{
          string tx=fun::PrintStr("      %s <%s>",v->name.c_str(),JChValues::TypeToStr(v->type).c_str());
          while(tx.size()<lenmax)tx=tx+".";
          Log->Printf("%s: %s",tx.c_str(),vtx.c_str());
        }
      }
    }
  }
}

//==============================================================================
/// Shows body configuration using Log.
//==============================================================================
void JChronoObjects::VisuBody(const JChBody *body)const{
  Log->Printf("  Body_%04u \"%s\" -  type: %s",body->Idb,body->IdName.c_str(),body->TypeToStr(body->Type).c_str());
  if(body->Type == JChBody::BD_Floating){
    Log->Printf("    MkBound......: %u",((const JChBodyFloating *)body)->MkBound);
    Log->Printf("    Mass.........: %g",body->GetMass());
    Log->Printf("    Center.......: (%s)",fun::Double3gStr(body->GetCenter()).c_str());
    const tmatrix3f inert=ToTMatrix3f(body->GetInertia());
    Log->Printf("    Inertia......: (%g,%g,%g) (xx,yy,zz)",inert.a11,inert.a22,inert.a33);
    Log->Printf("    Inertia......: (%g,%g,%g) (xy,yz,xz)",inert.a12,inert.a23,inert.a13);
    if(!body->GetMotionFree()){
      const tint3 m=body->GetTranslationFree();
      const tint3 r=body->GetRotationFree();
      Log->Printf("    MotionFree...: Translation:(%d,%d,%d) Rotation:(%d,%d,%d)",m.x,m.y,m.z,r.x,r.y,r.z);
    }
    if(body->GetLinearVelini()!=TFloat3(0) || body->GetAngularVelini()!=TFloat3(0)){
      const tfloat3 v=body->GetLinearVelini();
      const tfloat3 a=body->GetAngularVelini();
      if(v!=TFloat3(0))Log->Printf("    LinearVel0.: (%g,%g,%g) [m/s]",v.x,v.y,v.z);
      if(a!=TFloat3(0))Log->Printf("    AngularVel0: (%g,%g,%g) [rad/s]",a.x,a.y,a.z);
    }
  }
  if(body->Type==JChBody::BD_Moving){
    Log->Printf("    MkBound......: %u",((const JChBodyFixed *)body)->MkBound);
    Log->Printf("    Mass.........: %g",body->GetMass());
  }
  if(body->Type==JChBody::BD_Fixed){
    Log->Printf("    MkBound......: %u",((const JChBodyFixed *)body)->MkBound);
  }

  if(!body->GetModelFile().empty() && UseCollision){
    Log->Printf("    Kfric........: %g",body->GetKfric());
    Log->Printf("    Restitution..: %g",body->GetRestitu());
    if(UseChronoSMC){
      Log->Printf("    Young_Modulus: %g",body->GetYoung());
      Log->Printf("    Poisson Ratio: %g",body->GetPoisson());
    } 
    CheckParams(body); //Checks the above parameters in function of the contact method
    Log->Printf("    ModelFile....: %s",body->GetModelFile().c_str());
    Log->Printf("    ModelNormal..: %s",JChBody::NormalToStr(body->GetModelNormal()).c_str());
  }

  VisuValues(body->GetValuesPtr());
  if(body->GetLinkRefCount()){
    Log->Printf("    Links......: %u",body->GetLinkRefCount());
    for(unsigned c=0;c<body->GetLinkRefCount();c++){
      Log->Printf("      %s",body->GetLinkRef(c)->Name.c_str());
    }
  }
}

//==============================================================================
/// Shows link configuration using Log.
//==============================================================================
void JChronoObjects::VisuLink(const JChLink *link)const{
  Log->Printf("  Link \"%s\" -  type: %s",link->Name.c_str(),link->TypeToStr(link->Type).c_str());
  switch(link->Type){
    case JChLink::LK_Hinge:{
      const JChLinkHinge* linktype=(const JChLinkHinge*)link;
      Log->Printf("    Rotation point: (%s)",fun::Double3gStr(linktype->GetRotPoint()).c_str());
      Log->Printf("    Rotation axis.: (%s)",fun::Double3gStr(linktype->GetRotVector()).c_str());
    }break;
    case JChLink::LK_Spheric:{
      const JChLinkSpheric* linktype=(const JChLinkSpheric*)link;
      Log->Printf("    Rotation point: (%s)",fun::Double3gStr(linktype->GetRotPoint()).c_str());
    }break;
    case JChLink::LK_PointLine:{
      const JChLinkPointLine* linktype=(const JChLinkPointLine*)link;
      Log->Printf("    Sliding vector: (%s)",fun::Double3gStr(linktype->GetSlidingVector()).c_str());
      Log->Printf("    Rotation point: (%s)",fun::Double3gStr(linktype->GetRotPoint()).c_str());
      if(linktype->GetRotVector() !=TDouble3(0))Log->Printf("    Rotation axis.: (%s)",fun::Double3gStr(linktype->GetRotVector()).c_str());
      if(linktype->GetRotVector2()!=TDouble3(0))Log->Printf("    Rotation axis2: (%s)",fun::Double3gStr(linktype->GetRotVector2()).c_str());
    }break;
    case JChLink::LK_LinearSpring:{
      const JChLinkLinearSpring* linktype=(const JChLinkLinearSpring*)link;
      Log->Printf("    Point Body 1..: (%s)",fun::Double3gStr(linktype->GetPointfb0()).c_str());
      Log->Printf("    Point Body 2..: (%s)",fun::Double3gStr(linktype->GetPointfb1()).c_str());
      Log->Printf("    Rest length...: %g", linktype->GetRestLength());
    }break;
    case JChLink::LK_CoulombDamping:{
      const JChLinkCoulombDamping* linktype=(const JChLinkCoulombDamping*)link;
      Log->Printf("    Point Body 1..: (%s)",fun::Double3gStr(linktype->GetPointfb0()).c_str());
      Log->Printf("    Point Body 2..: (%s)",fun::Double3gStr(linktype->GetPointfb1()).c_str());
      Log->Printf("    Rest length...: %g", linktype->GetRestLength());
      Log->Printf("    CoulombDamping: %g", linktype->GetCoulombDamping());
    }break;
    case JChLink::LK_Pulley:{
      const JChLinkPulley* linktype=(const JChLinkPulley*)link;
      Log->Printf("    Rotation point: (%s)",fun::Double3gStr(linktype->GetRotPoint()).c_str());
      Log->Printf("    Rotation axis.: (%s)",fun::Double3gStr(linktype->GetRotVector()).c_str());
      Log->Printf("    Radius body1..: %g",linktype->GetRadius());
      Log->Printf("    Radius body2..: %g",linktype->GetRadius2());
    }break;
    default: Run_Exceptioon("Type of link is not supported.");
  }
  if(link->Type!=JChLink::LK_CoulombDamping && link->Type!=JChLink::LK_Pulley){
    Log->Printf("    Stiffness.....: %g",link->GetStiffness());
    Log->Printf("    Damping.......: %g",link->GetDamping());
  }
  VisuValues(link->GetValuesPtr());
  if(link->GetBodyRefCount()){
    Log->Printf("    Bodies........: %u",link->GetBodyRefCount());
    for(unsigned c=0;c<link->GetBodyRefCount();c++){
      Log->Printf("      Body_%04u \"%s\"",link->GetBodyRef(c)->Idb,link->GetBodyRef(c)->IdName.c_str());
    }
  }
}

//==============================================================================
/// Shows object configuration using Log.
//==============================================================================
void JChronoObjects::VisuConfig(std::string txhead, std::string txfoot)const{
  if(!txhead.empty())Log->Print(txhead);
  const JChronoData* chdata=ChronoLib->GetChronoData();
  Log->Printf("  DSPHChrono version: %s",ChronoLib->version.c_str());
  Log->Printf("  Data directory...: [%s]",chdata->GetDataDir().c_str());
  Log->Printf("  Collisions.......: %s",(UseCollision? "True": "False"));
  if(UseCollision){
    Log->Printf("  Collision dp.....: %g",chdata->GetCollisionDp());
    Log->Printf("  Contact Method...: %s",chdata->ContactMethodToStr().c_str());  //<chrono_contacts>
    Log->Printf("  Solver ..........: %s",chdata->SolverToStr().c_str());
  }
  Log->Printf("  Execution mode...: %s",chdata->GetMode().c_str());
  Log->Printf("  OpenMP Threads...: %d",chdata->GetOmpThreads());
  Log->Printf("  Bodies...........: %d",chdata->GetBodyCount());
  Log->Printf("  Links............: %u",chdata->GetLinkCount());
  //Log->Printf("  FEA Enabled......: %s",chdata->GetUseFEA()?"Yes":"No");     //<chrono_fea>

  for(unsigned c=0;c<chdata->GetBodyCount();c++)VisuBody(chdata->GetBody(c));
  for(unsigned c=0;c<chdata->GetLinkCount();c++)VisuLink(chdata->GetLink(c));
  //-Checks bodies without geometry for collisions.
  if(UseCollision){
    string mkboundlist;
    for(unsigned c=0;c<chdata->GetBodyCount();c++)if(chdata->GetBody(c)->GetModelFile().empty()){
      if(mkboundlist.empty())mkboundlist=fun::UintStr(chdata->GetBody(c)->MkBound);
      else mkboundlist=mkboundlist+","+fun::UintStr(chdata->GetBody(c)->MkBound);
    }
    if(!mkboundlist.empty())Log->PrintfWarning("The collisions with Chrono is activated but some bodies (mkbound=[%s]) do not have model file defined.",mkboundlist.c_str());
  }
  if(!txfoot.empty())Log->Print(txfoot);
}

//==============================================================================
/// Loads floating data to calculate coupling with Chrono.
//==============================================================================
void JChronoObjects::SetFtData(word mkbound,const tfloat3 &face,const tfloat3 &fomegaace){
  if(!ChronoLib->SetFtData(mkbound,face,fomegaace))Run_Exceptioon("Error running Chrono library.");
}

//<vs_fttvel_ini>
//==============================================================================
/// Loads imposed velocity for floating to calculate coupling with Chrono.
//==============================================================================
void JChronoObjects::SetFtDataVel(word mkbound,const tfloat3 &vlin,const tfloat3 &vang){
  if(!ChronoLib->SetFtDataVel(mkbound,vlin,vang))Run_Exceptioon("Error running Chrono library.");
}//<vs_fttvel_end>

//==============================================================================
/// Obtains floating data from coupling with Chrono.
//==============================================================================
void JChronoObjects::GetFtData(word mkbound,tdouble3 &fcenter,tfloat3 &fvel,tfloat3 &fomega)const{
  if(!ChronoLib->GetFtData(mkbound,fcenter,fvel,fomega))Run_Exceptioon("Error running Chrono library.");
}

//==============================================================================
/// Loads motion data to calculate coupling with Chrono.
//==============================================================================
void JChronoObjects::SetMovingData(word mkbound,bool simple,const tdouble3 &msimple,const tmatrix4d &mmatrix,double stepdt){
  if(!ChronoLib->SetMovingData(mkbound,simple,msimple,mmatrix,stepdt))Run_Exceptioon("Error running Chrono library.");
}

//==============================================================================
/// Computes a single timestep with Chrono for the system
//==============================================================================
void JChronoObjects::RunChrono(unsigned nstep,double timestep,double dt,bool predictor){
  if(!ChronoLib->RunChrono(timestep,dt,predictor))Run_Exceptioon("Error running Chrono library.");
  //-Saves floating body data in CSV files.
  if((LastTimeOk==timestep || NextTime<=timestep) && (SaveDataTime==0 || !predictor)){
    const JChronoData* chdata=ChronoLib->GetChronoData();
    for(unsigned cb=0;cb<chdata->GetBodyCount();cb++)if(chdata->GetBody(cb)->Type==JChBody::BD_Floating){
      const JChBodyFloating *body=(JChBodyFloating*)chdata->GetBody(cb);
      const string file=AppInfo.GetDirOut()+fun::PrintStr("ChronoExchange_mkbound_%u.csv",body->MkBound);
      jcsv::JSaveCsv2 scsv(file,true,AppInfo.GetCsvSepComa());
      if(!scsv.GetAppendMode()){
        Log->AddFileInfo("ChronoExchange_mkbound_XX.csv","Saves information of data exchange between DualSPHysics and Chrono library for each body.");
        //-Saves head.
        scsv.SetHead();
        scsv << "nstep;time [s];dt [s];predictor;face.x [m/s^2];face.y [m/s^2];face.z [m/s^2]";
        scsv << "fomegaace.x [rad/s^2];fomegaace.y [rad/s^2];fomegaace.z [rad/s^2];fvel.x [m/s];fvel.y [m/s];fvel.z [m/s]";
        scsv << "fcenter.x [m];fcenter.y [m];fcenter.z [m];fomega.x [rad/s];fomega.y [rad/s];fomega.z [rad/s]" << jcsv::Endl();
      }
      //-Saves data.
      scsv.SetData();
      scsv << nstep << timestep << dt << (predictor? "True": "False");
      scsv << body->GetInputFace();
      scsv << body->GetInputFomegaAce();
      scsv << body->GetOutputVel();
      scsv << body->GetOutputCenter();
      scsv << body->GetOutputOmega();
      scsv << jcsv::Endl();
      scsv.SaveData();
      //-Recalculates NextTime.
      if(LastTimeOk!=timestep){
        if(SaveDataTime>0)while(NextTime<=timestep)NextTime+=SaveDataTime;
        LastTimeOk=timestep;
      }
    }
    //-Saves forces for each body and link (link_forces.csv, body_forces.csv).
    ChronoLib->SaveForces();
  }
  //if(1){
  //  tdouble3 pcen;
  //  ChronoLib->GetBodyCenter("ball",pcen);
  //  Log->Printf("RunChrono----> timestep:%f  dt:%f  ball.center:(%f,%f,%f)",timestep,dt,pcen.x,pcen.y,pcen.z);
  //}
}

//==============================================================================
/// Saves special data for each PART.
//==============================================================================
void JChronoObjects::SavePart(int part){
  const double ds=Dp*SchemeScale;
  const JChronoData* chdata=ChronoLib->GetChronoData();
  //-Saves VTK of LinearSpring and CoulombDamping links.
  if(1){
    bool save=false;
    JVtkLib sh;
    for(unsigned c=0;c<chdata->GetLinkCount();c++){
      if(chdata->GetLink(c)->Type==JChLink::LK_LinearSpring || chdata->GetLink(c)->Type==JChLink::LK_CoulombDamping){
        const JChLink *link=chdata->GetLink(c);
        if(link->GetBodyRefCount()<1)Run_Exceptioon("Link without body reference.");
        const word mk=link->GetBodyRef(0)->MkBound+MkBoundFirst;
        tdouble3 p1,p2;
        if(ChronoLib->GetSpringLinkPositions(link->Name,p1,p2))Run_Exceptioon("Error running GetSpringLinkPositions() of Chrono library.");
        //Log->Printf("---> SpringPos: (%f,%f,%f) (%f,%f,%f)\n",p1.x,p1.y,p1.z,p2.x,p2.y,p2.z);
        const JChLink::StSaveSpring cfg=(link->Type==JChLink::LK_LinearSpring? ((const JChLinkLinearSpring*)link)->GetSvSpring(): ((const JChLinkCoulombDamping*)link)->GetSvSpring());
        if(cfg.nside>1){
          const double radius=cfg.radius,revlength=cfg.length;
          const int nsides=cfg.nside;
          const double cornersout=cfg.radius/2.f,cornersin=cfg.radius/4.f;
          const double restlen=ChronoLib->GetSpringLinkRestLength(link->Name); //-It is necessary when RestLength is variable.
          sh.AddShapeSpring(p1,p2,restlen,ds,cornersout,cornersin,radius,revlength,nsides,mk);
          save=true;
        }
        else if(cfg.nside==1){
          sh.AddShapeLine(p1,p2,mk);
          save=true;
        }
      }
    }
    if(save){
      Log->AddFileInfo("SpringVtk/Chrono_Springs_????.vtk","Saves VTK file with representation of Chrono springs.");
      const string filevtk=AppInfo.GetDirOut()+fun::FileNameSec("SpringVtk/Chrono_Springs.vtk",part);
      sh.SaveShapeVtk(filevtk,"Mk");
    }
  }
}

#endif




