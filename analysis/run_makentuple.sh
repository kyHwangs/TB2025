#!/bin/zsh

RUNLIST=(
12341
12343
12344
12345
)

for RUN in "${RUNLIST[@]}"; do
    echo "======================================="
    echo "Processing make_ntuple Run: ${RUN}"
    echo "======================================="
    
  ./make_ntuple --RunNumber ${RUN}

done
unset RUNLIST RUN
