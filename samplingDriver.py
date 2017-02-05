import os
import sys
import subprocess
import random
import commands
import time
import shutil
import glob


joblimit = 1
nodesOption = [4, 8, 16, 32, 64, 128, 256]
threadsOption = [2, 4, 8, 16]

for numOfNode in nodesOption:
    for nthread in threadsOption:
        os.system('mkdir -p /gpfs/u/home/ACME/ACMEtany/scratch/MSscale/playground/' + str(numOfNode) + '_' + str(nthread) + '/') 
        mode = 'small'
        timelimit = '60' 
        
        if numOfNode > 64: 
            mode = 'medium' 
            timelimit = '30'
        
        header = '#!/bin/bash \n#SBATCH -D /gpfs/u/home/ACME/ACMEtany/scratch/MSscale/playground/' + str(numOfNode) + '_' + str(nthread) + '/' + ' \n#SBATCH -t ' + timelimit + '\n#SBATCH --partition ' + mode + ' \n#SBATCH -N ' + str(numOfNode) + '\n#SBATCH -n ' + str(numOfNode*64) + '\n#SBATCH --overcommit \n' 
        tmn = commands.getstatusoutput('squeue -u ACMEtany | grep ACMEtany')[1].split('\n')
        size = 0
        if tmn != ['']:
            size = len(commands.getstatusoutput('squeue -u ACMEtany | grep ACMEtany')[1].split('\n')) 
        while size >= joblimit: 
            time.sleep(3)
            tmn = commands.getstatusoutput('squeue -u ACMEtany | grep ACMEtany')[1].split('\n')
            size = 0
            if tmn != ['']:
                size = len(commands.getstatusoutput('squeue -u ACMEtany | grep ACMEtany')[1].split('\n'))

            print "size is ", size

        file = header 
        f = open('runjob.sh', 'w', 1) # 3rd argv is the bufsize, 1 means line buffered
        
        file = file + 'srun /gpfs/u/home/ACME/ACMEtany/scratch/MSscale/MSthesis/q_MC.out --nonstop 2 voronmc.0000.dat 100 100 0 ' + str(nthread) + ' 1 723 723 1 1' 
        f.write(file) 
        f.close() 
        os.system('sbatch runjob.sh')
        time.sleep(3)

