#! /bin/bash

# To compile TBxxx.cc, type the commnand below
# `bash compile.sh TBxxx.cc` (or `bash compile.sh TBxxx.cc` for short)
# This will generate TBxxx executable

ext=${1##*.}
fname=`basename ${1} .${ext}`

echo "Compiling $fname.cc to $fname"
g++ -std=c++20 \
-I$INSTALL_DIR_PATH/include \
-I$YAMLPATH/../include \
-L$INSTALL_DIR_PATH/lib $INSTALL_DIR_PATH/lib64/libdrcTB.so $YAMLPATH/libyaml-cpp.so `root-config --cflags --libs` ${fname}.cc -o ${fname}
echo "Done!"