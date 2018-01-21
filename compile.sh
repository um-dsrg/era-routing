#!/bin/bash

script_path="$( cd "$(dirname "$0")" ; pwd -P )"

function build_debug {
  cd $script_path && mkdir -p build/debug
  cd $script_path && cd build/debug && cmake ../../ && make
}

function build_release {
  cd $script_path && mkdir -p build/release
  cd $script_path && cd build/release && cmake ../../ && make
}

if [ "$1" == debug ] ; then
  build_debug
elif [ "$1" == release ] ; then
  build_release
elif [ "$1" == clean ] ; then
  rm -rf build
else
  # Default show help
  build_debug
  build_release
fi

