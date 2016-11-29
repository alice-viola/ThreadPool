#!/bin/bash

DIR="$( cd "$( dirname "$0" )" && pwd )"

rm -rf $DIR/build
mkdir $DIR/build
cp Makefile ./build
cp threadpool.hpp ./build
cp test.hpp ./build
cp main.cpp ./build
cd $DIR/build
make ThreadPoolTest
