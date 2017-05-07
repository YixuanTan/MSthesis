import inotify.adapters
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import re
import sys
import matplotlib.animation as anim

regx = re.compile('.dat\\b')
plt.ion()
notifier = inotify.adapters.Inotify()
notifier.add_watch('/home/smartcoder/Documents/Code/MS/ColumnarGrowth/')
imgname = ""

#house keeping
subprocess.Popen("rm *.dat", shell=True)

for event in notifier.event_gen():
    if event is not None:
        print event      # uncomment to see all events generated
        if 'IN_CLOSE_WRITE' in event[1] and event[3].endswith(".dat"):
		    #print "file '{0}' created in '{1}'".format(event[3], event[2])
		    imgname = event[3]
		    subprocess.Popen("./mmsp2png " + imgname, shell=True, stderr=subprocess.STDOUT)
		    #subprocess.check_call("rm *.dat", shell=True)



			
