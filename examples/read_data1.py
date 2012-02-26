
import numpy as np
from textreader import readrows


filename = 'data/data1.txt'
f = open(filename, 'r')
# Skip the first line.
f.readline()

dt = np.dtype([('NAME', 'S8'), ('RATIO', np.float32), ('ALPHA', np.float32), ('BETA', np.float32)])
data1 = readrows(f, dt, delimiter='|', usecols=range(4))
print "data1.shape =", data1.shape
print "data1.dtype =", data1.dtype
print "data1:"
print data1
