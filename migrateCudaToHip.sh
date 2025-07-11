#!/usr/bin/env bash

VERB=YES
verbopt=""
if [ "${VERB}" == "YES" ]; then
  verbopt="-v"
fi

# Export paths of ROCm
export ROCM_DIR=/opt/rocm
export HIP_BIN="${ROCM_DIR}/hip/bin"
export HIP_PATH="${ROCM_DIR}/include"
export HIPIFY="${ROCM_DIR}/bin"
export THRUST_DIR="${ROCM_DIR}/include"
export ROCPRIM_DIR="${ROCM_DIR}/include"
export PATH="$PATH:$ROCM_DIR:$HIP_BIN:$THRUST_DIR:$ROCPRIM_DIR:$HIP_PATH:$HIPIFY"

# Source and output directories
dir_cuda="src/source"
dir_hip="src/source_hip"
dir_hip_res="__hip_resources"

# Remove dir_hip if it exists, then recreate it
if [ -d "${dir_hip}" ]; then
    rm -rf ${verbopt} "${dir_hip}"
fi

# Setup HIP source directory
mkdir -p ${verbopt} "${dir_hip}"
rm -f ${verbopt} "${dir_hip}"/*.cu "${dir_hip}"/*.h "${dir_hip}"/*.cpp "${dir_hip}"/*.txt "${dir_hip}"/*.prehip 2>/dev/null

# Copy source and HIP-specific resource files
cp -r ${verbopt} "${dir_cuda}/"* "${dir_hip}/"
cp -r ${verbopt} "${dir_hip_res}/"* "${dir_hip}/"

# Migrate CUDA to HIP
echo "Migrating CUDA to HIP..."

# Replace <cuda_runtime_api.h> with conditional HIP include
find ${dir_hip} -type f \( -name "*.cu" -o -name "*.cpp" -o -name "*.cuh" -o -name "*.h" -o -name "*.hpp" \) \
  -exec sed -i 's|#include <cuda_runtime_api.h>|#ifdef USE_HIP\n#include "hip/cuda_to_hip.h"\n#else\n#include <cuda_runtime_api.h>\n#endif|g' {} \;

# Preprocess source for HIP
find "${dir_hip}" -type f -exec sed -i 's/<math_constants.h>/"cuda\/math_constants.h"/g' {} \;
find "${dir_hip}" -type f -exec sed -i 's/hipHostAlloc/hipHostMalloc/g' {} \;
find "${dir_hip}" -type f -exec sed -i 's/\(g.overlap=deviceProp.deviceOverlap\)/\/\/\1/g' {} \;
find "${dir_hip}" -type f -exec sed -i 's/\(g.overlapcount=deviceProp.asyncEngineCount\)/\/\/\1/g' {} \;
find "${dir_hip}" -type f -exec sed -i 's/\(g.uva=deviceProp.unifiedAddressing\)/\/\/\1/g' {} \;


# Run hipify on all files
if command -v parallel &> /dev/null; then
    echo "Running hipconvertinplace-perl.sh with GNU Parallel..."
    find ${dir_hip} -type f \( -name "*.cpp" -o -name "*.cu" -o -name "*.h" \) \
      | parallel -j 16 ${HIPIFY}/hipconvertinplace-perl.sh
else
    echo "Running hipconvertinplace-perl.sh sequentially..."
    find ${dir_hip} -type f \( -name "*.cpp" -o -name "*.cu" -o -name "*.h" \) \
      -exec ${HIPIFY}/hipconvertinplace-perl.sh {} \;
fi

# Compile code
cd "${dir_hip}" || exit 1
make -f Makefile_hip -j 16
cd - || exit 1

