REM @echo off
rem %1 Execution mode: (cpu/gpu)  Eg: cpu
rem %2 Code version name.         Eg: win64
rem %3 Input directory.           Eg: _DataCases_Ds5Test
rem %4 Input file.                Eg: zSpheric2Dam_100K
rem %5 Log file.                  Eg: log.csv
rem %6 Execution label.           Eg: _hs3
rem %7 Extra execution options.   Eg: "-symplectic -ddt:3 -cellmode:h -sv:none"

set datacases=%3
set ds="../bin/windows/DualSPHysics%2.exe"

echo --- Run:%1_%2_%4%6 ---
echo --- %7 ---
set modo=%1
if %modo% == gpu:0 set modo=gpu
if %modo% == gpu:1 set modo=gpu
if %modo% == gpu:2 set modo=gpu

set dirout=Test%2_%modo%_%4%6
if exist %dirout% del /Q %dirout%\*.*
if exist %dirout%\data del /Q %dirout%\data\*.*
if not exist %dirout% mkdir %dirout%
if not exist %dirout% (
  echo Error: The output directory does not exist %dirout%
) else (
  copy %datacases%\%4*.xml %dirout%
  %ds% -name %datacases%/%4 -dirout %dirout% -dirdataout data -runname %dirout% -%1 -svres %7  
  if exist %5 call ctail.bat %dirout%\Run.csv 1 >> %5
  if not exist %5 type %dirout%\Run.csv > %5
)
echo --- Finished Ok:%1_%2_%4%6 ---
echo --- %7 ---
