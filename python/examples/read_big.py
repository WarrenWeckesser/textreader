
import time
import os
import numpy as np

from textreader import readrows


filename = 'data/big.csv'
if not os.path.exists(filename):
    print "{name} does not exist. Run the script generate_big.py to generate {name}.".format(name=filename)
else:
    dt = np.float32

    t0 = time.time()
    a = readrows(filename, dtype=dt, delimiter=',')
    t1 = time.time()
    print t1 - t0, "seconds"

    f = open(filename, 'r')
    first_two = f.readline().split(',')[:2]
    f.seek(-64, 2)
    last_two = f.readline().strip().split(',')[-2:]
    f.close()
    print "First two samples in the file:", first_two,  "  Data in array:", a[0,:2]
    print "Last two samples in the file:", last_two,  "  Data in array:", a[-1,-2:]
    print "Data has been converted to", dt
