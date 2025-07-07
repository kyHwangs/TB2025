#! /bin/bash

# To compile TBxxx.cc, type the commnand below
# `bash compile.sh TBxxx.cc` (or `bash compile.sh TBxxx.cc` for short)
# This will generate TBxxx executable

ext=${1##*.}
fname=`basename ${1} .${ext}`

echo "Compiling $fname.cc to $fname"
g++ \
-I$INSTALL_DIR_PATH/include \
-L$INSTALL_DIR_PATH/lib $INSTALL_DIR_PATH/lib/libdrcTB.dylib $YAMLPATH/libyaml-cpp.dylib `root-config --cflags --libs` ${fname}.cc -o ${fname}
echo "Done!"