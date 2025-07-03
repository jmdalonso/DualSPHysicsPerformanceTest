@echo off

cls

set flog=z_TestResults.csv

set vexe=_win64
set casedir=_DataCases_Ds5Test

REM set opts="-symplectic -ddt:3 -cellmode:h -tmax:0.001"
set opts="-symplectic -ddt:3 -cellmode:h -sv:none"

set mark=_hs3e_r01
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_04M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_07M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_13M  %flog% %mark% %opts%

set mark=_hs3e_r02
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_04M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_07M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_13M  %flog% %mark% %opts%

set mark=_hs3e_r03
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_04M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_07M  %flog% %mark% %opts%

set mark=_hs3e_r04
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_04M  %flog% %mark% %opts%

set mark=_hs3e_r05
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_04M  %flog% %mark% %opts%

set mark=_hs3e_r06
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_02M  %flog% %mark% %opts%

set mark=_hs3e_r07
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_01M  %flog% %mark% %opts%

set mark=_hs3e_r08
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%

set mark=_hs3e_r09
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%

set mark=_hs3e_r10
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_100K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_200K %flog% %mark% %opts%
call _rundsph5test.bat gpu %vexe% %casedir% zSpheric2Dam_400K %flog% %mark% %opts%

@echo --- DONE! ---
pause