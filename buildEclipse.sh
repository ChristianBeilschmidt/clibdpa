#!/bin/bash
# Script for building clibdpa on Linux machine

project=clibdpa

#expected build dir structure
buildexp=build/Eclipse_CDT4-Unix_Makefiles

currentdir=$PWD
builddir=./${buildexp}

mkdir -p ${builddir}

#get path to clibcdc libs
clibcdc=../clibcdc/${buildexp}
pushd ${clibcdc}
clibcdc=$PWD
popd

#get path to clibspi libs
clibspi=../clibspi/${buildexp}
pushd ${clibspi}
clibspi=$PWD
popd

#get path to cutils libs
cutils=../cutils/${buildexp}
pushd ${cutils}
cutils=$PWD
popd

#launch cmake to generate build environment
pushd ${builddir}
cmake -G "Eclipse CDT4 - Unix Makefiles" -Dclibcdc_DIR:PATH=${clibcdc} -Dclibspi_DIR:PATH=${clibspi} -Dcutils_DIR:PATH=${cutils} ${currentdir} -DCMAKE_BUILD_TYPE=Debug
popd

#build from generated build environment
cmake --build ${builddir}
