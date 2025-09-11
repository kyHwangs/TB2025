#! /bin/zsh

./make_uniformity --RunNumber 12217 --Case 1 --suffix $1
./make_uniformity --RunNumber 12218 --Case 2 --suffix $1
./make_uniformity --RunNumber 12219 --Case 3 --suffix $1
./make_uniformity --RunNumber 12220 --Case 3 --suffix $1
./make_uniformity --RunNumber 12221 --Case 4 --suffix $1
./make_uniformity --RunNumber 12222 --Case 5 --suffix $1

./make_uniformity --RunNumber 12223 --Case 10 --suffix $1
./make_uniformity --RunNumber 12225 --Case 9 --suffix $1
./make_uniformity --RunNumber 12226 --Case 8 --suffix $1
./make_uniformity --RunNumber 12227 --Case 7 --suffix $1
./make_uniformity --RunNumber 12228 --Case 6 --suffix $1

./make_uniformity --RunNumber 12229 --Case 11 --suffix $1
./make_uniformity --RunNumber 12230 --Case 12 --suffix $1
./make_uniformity --RunNumber 12231 --Case 13 --suffix $1
./make_uniformity --RunNumber 12232 --Case 14 --suffix $1
./make_uniformity --RunNumber 12233 --Case 15 --suffix $1

hadd ./UNIFORMITY/$1/TOTAL.root ./UNIFORMITY/$1/ROOT/*.root
 root -l -b -q 'plotter_uniformity.cc("'$1'")'

