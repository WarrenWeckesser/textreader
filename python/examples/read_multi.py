
import numpy as np
from textreader import readrows


filename = 'data/multi.dat'

f = open(filename, 'r')
line = f.readline()
while len(line) > 0:
    nrows = int(line)
    a = readrows(f, np.float32, numrows=nrows, sci='D', delimiter=',')
    print "a:"
    print a
    print
    line = f.readline()
