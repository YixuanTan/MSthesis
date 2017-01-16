#!/bin/bash
#
# srun command for CSCI-6360 group project.
# USAGE: /full/path/to/./q_MC.out [--help]
#                                 [--init dimension [outfile]]
#                                 [--nonstop dimension outfile steps [increment]]
#                                 [infile [outfile] steps [increment]]
#
#SBATCH --mail-type=END
#SBATCH --mail-user=tany3@rpi.edu
#SBATCH -D /gpfs/u/home/ACME/ACMEtany/scratch/A/5
#SBATCH --partition debug
#SBATCH -t 30
#SBATCH -N 32
#SBATCH -n 512
#SBATCH --overcommit
#SBATCH -o /gpfs/u/home/ACME/ACMEtany/scratch/A/5/xxx.log

srun --runjob-opts="--mapping TEDCBA" /gpfs/u/home/ACME/ACMEtany/barn/areaA/q_MC.out --nonstop 2 voronmc.0000.dat 1000 5 0 4 723 723 /gpfs/u/home/ACME/ACMEtany/barn/areaA/1000x660.txt

