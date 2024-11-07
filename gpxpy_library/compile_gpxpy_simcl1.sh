#!/bin/bash
################################################################################
set -e  # Exit immediately if a command exits with a non-zero status.
set -x  # Print each command before executing it.

################################################################################
# Configurations
################################################################################
# Load GCC compiler
#
module load clang/17.0.1
module load cuda/12.2.2

# Activate spack environment
source spack/share/spack/setup-env.sh
spack env activate gpxpy
# Set cmake command
export CMAKE_COMMAND=$(which cmake)
# Configure APEX
export APEX_SCREEN_OUTPUT=1
# Configure MKL
export MKL_CONFIG='-DMKL_ARCH=intel64 -DMKL_LINK=dynamic -DMKL_INTERFACE_FULL=intel_lp64 -DMKL_THREADING=sequential'

################################################################################
# Compile code
################################################################################
rm -rf build_gpxpy && mkdir build_gpxpy && cd build_gpxpy
# Configure the project
$CMAKE_COMMAND .. -DCMAKE_BUILD_TYPE=Release \
                  -DPYTHON_LIBRARY_DIR=$(python3 -c "import site; print(site.getsitepackages()[0])") \
                  -DPYTHON_EXECUTABLE=$(which python3) \
                  -DHPX_WITH_DYNAMIC_HPX_MAIN=ON \
                  -DCMAKE_C_COMPILER=$(which clang) \
                  -DCMAKE_CXX_COMPILER=$(which clang++) \
                  -DCMAKE_CUDA_COMPILER=$(which clang++) \
                  -DCMAKE_CUDA_FLAGS="--cuda-gpu-arch=sm_80 --cuda-path=/usr/local.nfs/sw/cuda/cuda-12.2.2" \
                  -DCMAKE_CUDA_ARCHITECTURES=80 \
                  ${MKL_CONFIG}
 # Build the project
make -j all
