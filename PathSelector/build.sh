#!/usr/bin/env bash

script_path="$( cd "$(dirname "$0")" ; pwd -P )"

function configureDebug {
  cd ${script_path} && mkdir -p build/debug
  cd ${script_path} && cd build/debug && cmake -G'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug  ../../
}

function configureRelease {
  cd ${script_path} && mkdir -p build/release
  cd ${script_path} && cd build/release && cmake -G'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release  ../../
}

function buildDebug {
  cd ${script_path}/build/debug && make
}

function buildRelease {
  cd ${script_path}/build/release && make
}

if [ "$1" == debug ] ; then
  configureDebug
  buildDebug
elif [ "$1" == configureDebug ] ; then
  configureDebug
elif [ "$1" == buildDebug ] ; then
  buildDebug
elif [ "$1" == release ] ; then
  configureRelease
  buildRelease
elif [ "$1" == configureRelease ] ; then
  configureRelease
elif [ "$1" == buildRelease ] ; then
  buildDebug
elif [ "$1" == clean ] ; then
  rm -rf build
else # Default build both debug and release
  configureDebug
  configureRelease

  buildDebug
  buildRelease
fi
