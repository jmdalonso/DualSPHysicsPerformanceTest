#!/bin/bash

#-Checks parameters
if (( $# != 7 )); then
  echo "ERROR: Invalid number of parameters $#."
  echo
  echo "List of parameters:"
  echo " 1. Execution mode: (cpu/gpu)  Eg: cpu"
  echo " 2. Code version name.         Eg: win64"
  echo " 3. Input directory.           Eg: _DataCases_Ds5Test"
  echo " 4. Input file.                Eg: zSpheric2Dam_100K"
  echo " 5. Log file.                  Eg: log.csv"
  echo " 6. Execution label.           Eg: _hs3"
  echo " 7. Extra execution options.   Eg: \"-symplectic -ddt:3 -cellmode:h -sv:none\""
  echo
  exit
fi

xexename=$2
xdircase=$3
xfilecase=$4
xlog=$5
xmark=$6
xopt=$7

ds="../bin/linux/DualSPHysics${xexename}"

#-Seleccion de modo
modo=g
if [ $1 = "cpu" -o $1 = "CPU" ]; then
  modo=c
fi

#-Genera directorio de salida
dirout=Test${xexename}_${modo}_${xfilecase}${xmark}
if [ -e $dirout ]; then
  # rm -f -r $dirout
  num=2
  diroutx=${dirout}"("${num}")"
  while [ -e ${diroutx} ]; do
    let num=$num+1
    diroutx=${dirout}"("${num}")"
  done
  dirout=${diroutx}
fi
mkdir ${dirout}
echo "dirout:[${dirout}]"
echo "xexename:[${xexename}]"
echo "ds:[${ds}]"
echo " "

#-Ejecucion de simulacion
if [ -e $dirout ]; then
  if [ -x $ds ]; then
    cp ${xdircase}/${xfilecase}.xml $dirout/
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(pwd)
    ./${ds} -name ${xdircase}/${xfilecase} -dirout $dirout -dirdataout data -runname $dirout -${1} -svres ${xopt}
    if [ -e ${xlog} ]; then
      tail -n 1 $dirout/Run.csv >> ${xlog}
    else
      cat $dirout/Run.csv > ${xlog}
    fi
  else
    echo "ERROR: Executable file is missing."
    exit
  fi
else 
  echo "ERROR: Output directory is missing."
  exit
fi

echo --- Finished Ok:${xexename}_$1_${xdircase}_${xfilecase}${xmark} ---
echo --- ${xopt} ---



