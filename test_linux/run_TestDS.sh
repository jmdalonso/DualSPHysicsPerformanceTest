#!/bin/bash

flog=z_TestResults.csv

vexe=Amd_linux64
casedir=_DataCases_Ds5Test

# opt="-symplectic -ddt:3 -cellmode:h -tmax:0.001"
opt="-symplectic -ddt:3 -cellmode:h -sv:none"

mark=_hs3e_r01
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_04M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_07M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_13M $flog $mark "$opt"

mark=_hs3e_r02
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_04M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_07M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_13M $flog $mark "$opt"

mark=_hs3e_r03
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_04M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_07M $flog $mark "$opt"

mark=_hs3e_r04
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_04M $flog $mark "$opt"

mark=_hs3e_r05
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_04M $flog $mark "$opt"

mark=_hs3e_r06
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_02M $flog $mark "$opt"

mark=_hs3e_r07
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_01M $flog $mark "$opt"

mark=_hs3e_r08
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"

mark=_hs3e_r09
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"

mark=_hs3e_r10
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_100K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_200K $flog $mark "$opt"
./_rundsph5test.sh gpu $vexe $casedir zSpheric2Dam_400K $flog $mark "$opt"


echo "--- DONE! ---"

