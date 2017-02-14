import os

# traverse root directory, and list directories as dirs and files as files
out = open('scalability.txt', 'w')
for root, dirs, files in os.walk("/gpfs/u/scratch/ACME/ACMEtany/MSscale/playground/"):
#    for d in dirs:
#            print "folder : ", os.path.join(root, d)
#            print "root : ", root
    for f in files:
        if f.startswith('slurm'):
            print "file   : ", os.path.join(root, f)
            filename = os.path.join(root, f)
            node = root.split('/')[-1].split('_')[0]
            print 'node ', node
            pthread = root.split('/')[-1].split('_')[1]
            print 'pthread ', pthread
            myfile = [line.strip() for line in open(filename, 'r')]
            exetime = list(myfile)[-1].split()[-1]                                                                                                
            print 'exetime ', exetime 
            rank = myfile[2].split()[-2]     
            out.write(node + ' ' + rank + ' ' + pthread + ' ' + exetime + '\n')

#    with open('test.txt') as myfile:
#        print (list(myfile)[-1])
