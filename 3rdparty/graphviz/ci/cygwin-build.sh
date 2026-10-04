#!/usr/bin/env bash

if [ -z ${CI+x} ]; then
  echo "this script is only intended to run in CI" >&2
  exit 1
fi

set -e
set -o pipefail
set -u
set -x

windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages autoconf2.5
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages automake
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages bison
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages cmake
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages flex
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages gcc-core
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages gcc-g++
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages git
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages libcairo-devel
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages libexpat-devel
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages libpango1.0-devel
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages libgd-devel
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages libtool
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages make
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages python3
windows/dependencies/graphviz-build-utilities/cygwin/setup-x86_64.exe --quiet-mode --wait --packages zlib-devel

# Use the libs installed with cygwinsetup instead of those in
# https://gitlab.com/graphviz/graphviz-windows-dependencies. Also disable GVEdit
# because we do not have Qt installed.
export CMAKE_OPTIONS="-Duse_win_pre_inst_libs=OFF -DWITH_GVEDIT=OFF"
export CMAKE_OPTIONS="$CMAKE_OPTIONS -DENABLE_LTDL=ON"
export CMAKE_OPTIONS="$CMAKE_OPTIONS -DWITH_EXPAT=ON"
export CMAKE_OPTIONS="$CMAKE_OPTIONS -DWITH_ZLIB=ON"

# make Git running under the Cygwin user trust files owned by the Windows user
git config --global --add safe.directory $(pwd)

ci/build.sh
