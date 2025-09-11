#! /bin/bash

source envset.sh
mkdir build install
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../install 
make -j4 install
cd ..
source envset.sh
cd analysis
./compile.sh make_ntuple 
./compile.sh make_summary
chmod +x run_makeResolution.sh
