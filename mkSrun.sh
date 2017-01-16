#!/bin/bash

# configs  change to your desired configurations:
#N ; number of node
#n ; number of rank
#p ; number of thread


configs=(
#"1     200     523 "
#"2     250     523 "
#"3     300     523 "
#"4     350     523 "
#"5     400    523 "
"6     500    523 "
"7    600     523 "
"8     700    523 "
"9    800     523 "
"10     900    523 "
)

rm mc.*.sh

for conf in "${configs[@]}" ; do
    nfolder=`echo $conf | awk '{print $1}'`
    velinv=`echo $conf | awk '{print $2}'`
    peaktmp=`echo $conf | awk '{print $3}'`

    rm /gpfs/u/scratch/ACME/ACMEtany/JumpMove/$nfolder/*  
    mkdir /gpfs/u/scratch/ACME/ACMEtany/JumpMove/$nfolder

    sed -e "s/NFOLDER/$nfolder/g" \
        -e "s/VELINV/$velinv/g" \
        -e "s/PEAKTMP/$peaktmp/g" \
        srun_MC.tmp > mc.$velinv.$peaktmp.sh

done