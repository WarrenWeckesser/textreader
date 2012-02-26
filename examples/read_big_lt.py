
import time
import os
import numpy as np

from textreader import readrows


filename = 'data/big.csv'
if not os.path.exists(filename):
    print "{name} does not exist. Run the script generate_big.py to generate {name}.".format(name=filename)
else:
    fields = [('f'+str(k), np.float32) for k in range(50)]
    dt = np.dtype(fields)

    t0 = time.time()
    a = np.loadtxt(filename, delimiter=',')
    t1 = time.time()
    print t1 - t0