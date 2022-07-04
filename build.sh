#!/bin/bash
sudo apt-get install cmake
sudo apt-get install libmysqlclient-dev
sudo apt-get install libssl-dev

mkdir -p build
cd build || exit
cmake ..
make -j12
sudo make install