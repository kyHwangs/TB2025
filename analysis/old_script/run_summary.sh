#! /bin/zsh

# ./make_summary --suffix M5T2_EMRes_1tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --module M5-T2
# ./make_summary --suffix M5T2_EMRes_4tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --module M2-T3 M2-T4 M5-T1 M5-T2
# ./make_summary --suffix M5T2_EMRes_9tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --module M5-T2 M5-T1 M5-T3 M5-T4 M6-T1 M6-T3 M2-T3 M2-T4 M3-T3
# ./make_summary --suffix M5T2_EMRes_16tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --module M1-T2 M1-T4 M2-T1 M2-T2 M2-T3 M2-T4 M3-T1 M3-T3 M4-T2 M4-T4 M5-T1 M5-T2 M5-T3 M5-T4 M6-T1 M6-T3
# ./make_summary --suffix M5T2_EMRes_25tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --ignore M1-T1 M1-T3 M4-T1 M4-T3 M7-T1 M7-T3 M7-T4 M8-T3 M8-T4 M9-T3 M9-T4
# ./make_summary --suffix M5T2_EMRes_36tower_4mmCorr --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 

# ./make_summary --suffix M5T2_Center_Sur --set 12263 12260 12265 12269 12271 12273 12275 12277 --energy 120 100 80 60 40 30 20 10 --ignore M5-T2 M5-T1 M5-T3 M5-T4 M6-T1 M6-T3 M2-T3 M2-T4 M3-T3

# ./make_summary --suffix M5T3_100GeV --set 12229 --energy 100 --module M5-T3
# ./make_summary --suffix M5T3_100GeV --set 12193 --energy 60 --module M5-T3

# ./make_summary --suffix NominalScaleFactor --set 12269 --energy 60

./make_summary_hadron --suffix M5T2_Hadron_test --energy 20 40 60 80 100 120 


