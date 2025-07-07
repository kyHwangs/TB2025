#! /bin/bash

# When using CVMFS (no X11)
# source /cvmfs/sft.cern.ch/lcg/views/LCG_102/arm64-mac12-clang131-opt/setup.sh

# When using local ROOT (X11 activated)
source /Users/yhep/scratch/DQM/root-6.26.04/install/bin/thisroot.sh

export INSTALL_DIR_PATH=$PWD/install

export PATH=$INSTALL_DIR_PATH/lib:$PATH
export LD_LIBRARY_PATH=$INSTALL_DIR_PATH/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$INSTALL_DIR_PATH/lib:$DYLD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH:$INSTALL_DIR_PATH/lib
export YAMLPATH=/cvmfs/sft.cern.ch/lcg/releases/yamlcpp/0.6.3-d05b2/arm64-mac12-clang131-opt/lib
