#DualSPHysics GPU/CPU v5.0.112 02-07-2025

#=============== Compilation Options (YES/NO) ===============
USE_DEBUG=NO
USE_FAST_MATH=YES
USE_NATIVE_CPU_OPTIMIZATIONS=NO
COMPILE_VTKLIB=NO
COMPILE_NUMEXLIB=NO
COMPILE_CHRONO=NO
COMPILE_WAVEGEN=NO
COMPILE_MOORDYN=NO

LIBS_DIRECTORIES=-L./

EXECNAME=DualSPHysics_linux64
EXECS_DIRECTORY=../../bin/linux

# -std=c++0x ---> Used to avoid errors for calls to enums
ifeq ($(USE_DEBUG), YES)
  CCFLAGS=-c -O0 -g -Wall -fopenmp -D_WITHGPU -std=c++0x
else
  CCFLAGS=-c -O3 -fopenmp -D_WITHGPU -std=c++0x
  ifeq ($(USE_FAST_MATH), YES)
    CCFLAGS+= -ffast-math
  endif
  ifeq ($(USE_NATIVE_CPU_OPTIMIZATIONS), YES)
    CCFLAGS+= -march=native
  endif
endif
CC=g++
CCLINKFLAGS=-fopenmp -lgomp

ifeq ($(COMPILE_VTKLIB), NO)
  CCFLAGS:=$(CCFLAGS) -DDISABLE_VTKLIB
endif
ifeq ($(COMPILE_NUMEXLIB), NO)
  CCFLAGS:=$(CCFLAGS) -DDISABLE_NUMEXLIB
endif
ifeq ($(COMPILE_CHRONO), NO)
  CCFLAGS:=$(CCFLAGS) -DDISABLE_CHRONO
endif
ifeq ($(COMPILE_WAVEGEN), NO)
  CCFLAGS:=$(CCFLAGS) -DDISABLE_WAVEGEN
endif
ifeq ($(COMPILE_MOORDYN), NO)
  CCFLAGS:=$(CCFLAGS) -DDISABLE_MOORDYN
endif

#=============== CUDA selection ===============
CUDAVER=11

ifneq ($(CUDA),)
  CUDAVER=$(CUDA)
endif

#=============== CUDA toolkit directory (make appropriate for local CUDA installation) ===============
ifeq ($(CUDAVER),00)
  DIRTOOLKIT=/usr/local/cuda
endif
ifeq ($(CUDAVER),92)
  DIRTOOLKIT=/exports/opt/NVIDIA/cuda-9.2
endif
ifeq ($(CUDAVER),11)
  DIRTOOLKIT=/exports/opt/NVIDIA/cuda-11.7
endif
ifeq ($(CUDAVER),12)
  DIRTOOLKIT=/exports/opt/NVIDIA/cuda-12.8
endif

#=============== Select GPU architectures ===============
ifeq ($(CUDAVER),92)
  # module load cuda/9.2
  GENCODE:=$(GENCODE) -gencode=arch=compute_30,code=\"sm_30,compute_30\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_35,code=\"sm_35,compute_35\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_50,code=\"sm_50,compute_50\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_61,code=\"sm_61,compute_61\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_70,code=\"sm_70,compute_70\"
endif
ifeq ($(CUDAVER),11)
  # module load cuda/11.7
  GENCODE:=$(GENCODE) -gencode=arch=compute_35,code=\"sm_35,compute_35\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_50,code=\"sm_50,compute_50\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_61,code=\"sm_61,compute_61\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_70,code=\"sm_70,compute_70\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_80,code=\"sm_80,compute_80\"
  GENCODE:=$(GENCODE) -gencode=arch=compute_86,code=\"sm_86,compute_86\"
endif


#=============== Files to compile ===============
OBJXML=JXml.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o
OBJSPHMOTION=JMotion.o JMotionList.o JMotionMov.o JMotionObj.o JMotionPos.o JSphMotion.o
OBCOMMON=Functions.o FunctionsGeo3d.o JAppInfo.o JBinaryData.o JDataArrays.o JException.o JLinearValue.o JLog2.o JMeanValues.o JObject.o JOutputCsv.o JRadixSort.o JRangeFilter.o JReadDatafile.o JSaveCsv2.o JTimeControl.o randomc.o
OBCOMMONDSPH=JDsphConfig.o JPartDataBi4.o JPartDataHead.o JPartFloatBi4.o JPartOutBi4Save.o JSpaceCtes.o JSpaceEParms.o JSpaceParts.o JSpaceProperties.o JSpaceUserVars.o JSpaceVtkOut.o
OBSPH=JArraysCpu.o JCellDivCpu.o JCfgRun.o JDamping.o JGaugeItem.o JGaugeSystem.o JPartsOut.o JSaveDt.o JShifting.o JSph.o JSphAccInput.o JSphCpu.o JSphInitialize.o JSphMk.o JSphPartsInit.o JSphDtFixed.o JSphVisco.o JTimeOut.o JWaveSpectrumGpu.o main.o
OBSPHSINGLE=JCellDivCpuSingle.o JPartsLoad4.o JSphCpuSingle.o
OBCOMMONGPU=FunctionsCuda.o JObjectGpu.o 
OBSPHGPU=JArraysGpu.o JDebugSphGpu.o JCellDivGpu.o JSphGpu.o 
OBSPHSINGLEGPU=JCellDivGpuSingle.o JSphGpuSingle.o
OBCUDA=JCellDivGpu_ker.o JCellDivGpuSingle_ker.o JGauge_ker.o JReduSum_ker.o JShifting_ker.o JSphAccInput_ker.o JSphGpu_ker.o JSphGpuSimple_ker.o JWaveOrder2_ker.o

OBWAVERZ=JMLPistonsGpu.o JRelaxZonesGpu.o
OBWAVERZCUDA=JRelaxZone_ker.o
OBCHRONO=JChronoObjects.o
OBMOORDYN=JMooredFloatings.o JSphFtForcePoints.o
OBINOUT=JSphCpu_InOut.o JSphCpuSingle_InOut.o JSphBoundCorr.o JSphInOut.o JSphInOutZone.o JSphInOutGridData.o JSphInOutPoints.o JSimpleNeigs.o
OBINOUTGPU=JSphGpuSingle_InOut.o
OBMDBC=JPartNormalData.o JNormalsMarrone.o

OBJECTS=$(OBJXML) $(OBJSPHMOTION) $(OBCOMMON) $(OBCOMMONDSPH) $(OBSPH) $(OBSPHSINGLE)
OBJECTS:=$(OBJECTS) $(OBCOMMONGPU) $(OBSPHGPU) $(OBSPHSINGLEGPU) $(OBCUDA)
OBJECTS:=$(OBJECTS) $(OBWAVERZ) $(OBWAVERZCUDA) $(OBCHRONO) $(OBMOORDYN) $(OBINOUT) $(OBINOUTGPU) $(OBMDBC)

#=============== DualSPHysics libs to be included ===============
JLIBS=${LIBS_DIRECTORIES}

#=============== GPU Code Compilation ===============
CCFLAGS := $(CCFLAGS) -I./ -I$(DIRTOOLKIT)/include
CCLINKFLAGS := $(CCLINKFLAGS) -L$(DIRTOOLKIT)/lib64 -lcudart_static -ldl -lrt
NCC=nvcc
ifeq ($(USE_DEBUG), NO)
  NCCFLAGS+=-c $(GENCODE) -O3 -ccbin $(CC)
else
  NCCFLAGS+=-c $(GENCODE) -O0 -ccbin $(CC) -g
endif
ifeq ($(USE_FAST_MATH), YES)
  NCCFLAGS+= -use_fast_math
endif

all:$(EXECS_DIRECTORY)/$(EXECNAME) 
	rm -rf *.o
ifeq ($(USE_DEBUG), NO)
	@echo "  --- Compiled Release GPU version ---"
else
	@echo "  --- Compiled Debug GPU version ---"
	mv $(EXECS_DIRECTORY)/$(EXECNAME) $(EXECNAME)_debug
endif

$(EXECS_DIRECTORY)/$(EXECNAME):  $(OBJECTS)
	$(CC) $(OBJECTS) $(CCLINKFLAGS) -o $@ $(JLIBS)

.cpp.o: 
	$(CC) $(CCFLAGS) $< 

JSphGpu_ker.o: JSphGpu_ker.cu
	$(NCC) $(NCCFLAGS) JSphGpu_ker.cu

JSphGpuSimple_ker.o: JSphGpuSimple_ker.cu
	$(NCC) $(NCCFLAGS) JSphGpuSimple_ker.cu

JCellDivGpu_ker.o: JCellDivGpu_ker.cu
	$(NCC) $(NCCFLAGS) JCellDivGpu_ker.cu

JCellDivGpuSingle_ker.o: JCellDivGpuSingle_ker.cu
	$(NCC) $(NCCFLAGS) JCellDivGpuSingle_ker.cu

JShifting_ker.o: JShifting_ker.cu
	$(NCC) $(NCCFLAGS) JShifting_ker.cu

JSphAccInput_ker.o: JSphAccInput_ker.cu
	$(NCC) $(NCCFLAGS) JSphAccInput_ker.cu

JGauge_ker.o: JGauge_ker.cu
	$(NCC) $(NCCFLAGS) JGauge_ker.cu

JWaveOrder2_ker.o: JWaveOrder2_ker.cu
	$(NCC) $(NCCFLAGS) JWaveOrder2_ker.cu

JReduSum_ker.o: JReduSum_ker.cu
	$(NCC) $(NCCFLAGS) JReduSum_ker.cu

JRelaxZone_ker.o: JRelaxZone_ker.cu
	$(NCC) $(NCCFLAGS) JRelaxZone_ker.cu

clean:
	rm -rf *.o $(EXECNAME) $(EXECNAME)_debug

