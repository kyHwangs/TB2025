#! /bin/sh

mkdir Run_6034
mkdir -p Run_6034/Run_6034_Fast
mkdir -p Run_6034/Run_6034_Wave

for i in {1..20}
do
  mkdir -p Run_6034/Run_6034_Fast/Run_6034_Fast_MID_$i
  mkdir -p Run_6034/Run_6034_Wave/Run_6034_Wave_MID_$i
done
