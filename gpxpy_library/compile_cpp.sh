#!/bin/bash

################################################################################
# Build GPXPy C++ library
################################################################################

# exit on error (non-zero status); print each command before execution
set -ex

# Configuration ----------------------------------------------------------------

# Load spack environment
source $HOME/spack/share/spack/setup-env.sh
spack env activate gpxpy

# Set cmake command
export CMAKE_COMMAND=$(which cmake)

# Activate APEX output to stdout
export APEX_SCREEN_OUTPUT=1

# Configure MKL
export MKL_CONFIG='-DMKL_ARCH=intel64 -DMKL_LINK=dynamic -DMKL_INTERFACE_FULL=intel_lp64 -DMKL_THREADING=sequential'

# ------------------------------------------------------------------------------

# Compile to  Compile to ./build_cpp -------------------------------------------

# Reset build directory
rm -rf build_cpp && mkdir build_cpp && cd build_cpp

# Configure the project
$CMAKE_COMMAND ../core \
    -DCMAKE_BUILD_TYPE=Release \
    -DHPX_WITH_DYNAMIC_HPX_MAIN=ON \
    -DCMAKE_C_COMPILER=$(which gcc) \
    -DCMAKE_CXX_COMPILER=$(which g++) \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    ${MKL_CONFIG}

# Build project
make -j all
make install

# ------------------------------------------------------------------------------