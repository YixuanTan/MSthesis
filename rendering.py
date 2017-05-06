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
#fig = plt.figure()

def plot_cont():    
    def update(i):
	    img = mpimg.imread(regx.sub('.png', imgname))
	    imgplot = plt.imshow(img)

    a = anim.FuncAnimation(fig, update, frames=100, repeat=False)
    plt.show()
    plt.pause(1)

for event in notifier.event_gen():
    if event is not None:
        print event      # uncomment to see all events generated
        if 'IN_CLOSE_WRITE' in event[1] and event[3].endswith(".dat"):
		    #print "file '{0}' created in '{1}'".format(event[3], event[2])
		    imgname = event[3]
		    #time.sleep(0.1)
		    response = subprocess.check_output("./mmsp2png " + imgname, shell=True, stderr=subprocess.STDOUT)
		    #print response
		    #plot_cont()
		    #subprocess.check_call("rm *.dat", shell=True)



			
