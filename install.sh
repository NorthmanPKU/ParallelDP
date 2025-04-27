#!/bin/bash
set -e

echo "Bootstrapping OpenCilk + project build..."

# Install OpenCilk
if [ ! -x build/opencilk/bin/clang++ ]; then
  echo "Building OpenCilk..."

  # if exists
  if [ -d "deps/infrastructure" ]; then
    echo "OpenCilk already cloned"
  else
    git clone --depth=1 --branch opencilk/v2.1 https://github.com/OpenCilk/infrastructure deps/infrastructure
  fi
  echo "Getting OpenCilk..."
  if [ -d "deps/opencilk" ]; then
    echo "deps/opencilk already exists"
  else
    bash deps/infrastructure/tools/get $(pwd)/deps/opencilk
  fi
  echo "Patching build script..."
  sed -i 's/OPENCILK_RUNTIMES="cheetah;cilktools"/OPENCILK_RUNTIMES="cheetah;cilktools;openmp"/' deps/infrastructure/tools/build
  echo "Building..."
  bash deps/infrastructure/tools/build $(pwd)/deps/opencilk $(pwd)/build/opencilk
else
  echo "OpenCilk already built"
fi

# Install Paralay
if [ -d "build/parlay" ] || pkg-config --exists parlay; then
  echo "Parlay already available"
else
  echo "Installing Parlay..."
  # Clone Parlay repository
  if [ ! -d "deps/parlay" ]; then
    git clone https://github.com/cmuparlay/parlaylib.git deps/parlay
  fi
  
  # Create build directory and install to local
  mkdir -p deps/parlay/build
  cd deps/parlay/build
  cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$(pwd)/../../../build/parlay
  cmake --build . --target install
  cd ../../../
  echo "Parlay installed to build/parlay"
fi


echo "Configuring with OpenCilk compiler..."
cmake -B build -S . \
  -DCMAKE_C_COMPILER=$(pwd)/build/opencilk/bin/clang \
  -DCMAKE_CXX_COMPILER=$(pwd)/build/opencilk/bin/clang++ \
  -DCMAKE_CXX_FLAGS="-fopencilk" \
  -DCMAKE_PREFIX_PATH=$(pwd)/build/parlay

echo "Building project..."
cmake --build build
