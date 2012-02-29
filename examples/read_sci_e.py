
import numpy as np
from textreader import readrows


filename = 'data/sci_e.csv'

dt = np.dtype([('x', np.float64), ('y', np.float64), ('z', np.float64)])
a = readrows(filename, dt, delimiter=',')
print "a.shape =", a.shape
print "a.dtype =", a.dtype
print "a:"
print a
print
