#!/bin/bash

export gen="../bin/linux/GenCase_linux64"

export dircase=_DataCases_Ds5Test
export case=Spheric2Dam

rm ${dircase}/z*.*

${gen} ${dircase}/${case}_100K ${dircase}/z${case}_100K
${gen} ${dircase}/${case}_200K ${dircase}/z${case}_200K
${gen} ${dircase}/${case}_400K ${dircase}/z${case}_400K
${gen} ${dircase}/${case}_01M ${dircase}/z${case}_01M
${gen} ${dircase}/${case}_02M ${dircase}/z${case}_02M
${gen} ${dircase}/${case}_04M ${dircase}/z${case}_04M
${gen} ${dircase}/${case}_07M ${dircase}/z${case}_07M
${gen} ${dircase}/${case}_13M ${dircase}/z${case}_13M

rm ${dircase}/*_hdp_Actual.vtk
rm ${dircase}/*_Actual.vtk

@echo --- DONE! ---
