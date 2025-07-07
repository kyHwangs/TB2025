#! /bin/sh

for i in {1..20}
do
    cp fast_sample.dat Run_6034/Run_6034_Fast/Run_6034_Fast_MID_$i/Run_6034_Fast_MID_${i}_FILE_${1}.dat
    cp wave_sample.dat Run_6034/Run_6034_Wave/Run_6034_Wave_MID_$i/Run_6034_Wave_MID_${i}_FILE_${1}.dat
done
