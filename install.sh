#!/bin/bash
set -e

echo "Bootstrapping OpenCilk + project build..."


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

echo "Configuring with OpenCilk compiler..."
cmake -B build -S . \
  -DCMAKE_C_COMPILER=$(pwd)/build/opencilk/bin/clang \
  -DCMAKE_CXX_COMPILER=$(pwd)/build/opencilk/bin/clang++ \
  -DCMAKE_CXX_FLAGS="-fopencilk"

echo "Building project..."
cmake --build build
