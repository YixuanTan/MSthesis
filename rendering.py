import inotify.adapters
import subprocess as subp
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import re

regx = re.compile('.dat\\b')
plt.ion()
notifier = inotify.adapters.Inotify()
notifier.add_watch('/home/smartcoder/Documents/Code/MS/ColumnarGrowth/')

for event in notifier.event_gen():
    if event is not None:
        print event      # uncomment to see all events generated
        if 'IN_CREATE' in event[1] and event[3].endswith(".dat"):
		    #print "file '{0}' created in '{1}'".format(event[3], event[2])
		    imgname = event[3]
		    print 'there'
		    subp.check_call("./mmsp2png " + imgname, shell=True)
		    print 'hello1'
		    img = mpimg.imread(regx.sub('.png', imgname))
		    imgplot = plt.imshow(img)
		    print 'hello2'
		    plt.pause(0.001)
		    plt.show(block=True) # block=True lets the window stay open at the end of the animation.
		    subp.check_call("rm *.png", shell=True)

			
