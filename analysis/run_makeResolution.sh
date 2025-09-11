#! /bin/sh

# base directory
# -------------------------------------------------------------------------------
BASEDIR=$1

echo -e "-----------------------------------------------------------"
echo -e "STARTING EM ENERGY RESOLUTION: $BASEDIR"
echo -e "-----------------------------------------------------------"

mkdir -p ./RESULT/$BASEDIR/RAW

## EM energy resolution runs @ M5T2
## START ------------------------------------------------------------------------

### make ntuple from RAW data
### START =======================================================================

#### 120 GeV
./make_ntuple --RunNumber 12263
./make_ntuple --RunNumber 12264
./make_ntuple --RunNumber 12267
./make_ntuple --RunNumber 12268
./make_ntuple --RunNumber 12279
./make_ntuple --RunNumber 12280

#### 100 GeV
./make_ntuple --RunNumber 12260
./make_ntuple --RunNumber 12261

#### 80 GeV
./make_ntuple --RunNumber 12265
./make_ntuple --RunNumber 12266

#### 60 GeV
./make_ntuple --RunNumber 12269
./make_ntuple --RunNumber 12270

#### 40 GeV
./make_ntuple --RunNumber 12271
./make_ntuple --RunNumber 12272

#### 30 GeV
./make_ntuple --RunNumber 12273
./make_ntuple --RunNumber 12274

#### 20 GeV
./make_ntuple --RunNumber 12275
./make_ntuple --RunNumber 12276

#### 10 GeV
./make_ntuple --RunNumber 12277

hadd ./RESULT/$BASEDIR/RAW/e_120GeV.root ./RAW/Run12263.root ./RAW/Run12264.root ./RAW/Run12267.root ./RAW/Run12268.root ./RAW/Run12279.root ./RAW/Run12280.root 
hadd ./RESULT/$BASEDIR/RAW/e_100GeV.root ./RAW/Run12260.root ./RAW/Run12261.root
hadd ./RESULT/$BASEDIR/RAW/e_80GeV.root ./RAW/Run12265.root ./RAW/Run12266.root
hadd ./RESULT/$BASEDIR/RAW/e_60GeV.root ./RAW/Run12269.root ./RAW/Run12270.root
hadd ./RESULT/$BASEDIR/RAW/e_40GeV.root ./RAW/Run12271.root ./RAW/Run12272.root
hadd ./RESULT/$BASEDIR/RAW/e_30GeV.root ./RAW/Run12273.root ./RAW/Run12274.root
hadd ./RESULT/$BASEDIR/RAW/e_20GeV.root ./RAW/Run12275.root ./RAW/Run12276.root
hadd ./RESULT/$BASEDIR/RAW/e_10GeV.root ./RAW/Run12277.root

### END =========================================================================

./make_summary --suffix $BASEDIR --energy 10 20 30 40 60 80 100 120

## END --------------------------------------------------------------------------

echo -e "-----------------------------------------------------------"
echo -e "DONE! EM ENERGY RESOLUTION: $BASEDIR"
echo -e "SUMMARY:"
echo -e "    RESOLUTION PLOT: ./RESULT/$BASEDIR/RESOLUTION/"
echo -e "    ENERGY PLOT: ./RESULT/$BASEDIR/ROOT/"
echo -e "-----------------------------------------------------------"


