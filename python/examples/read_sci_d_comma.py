
import numpy as np
from textreader import readrows


filename = 'data/sci_d_comma.csv'

dt = np.dtype([('w', np.float64), ('x', np.float64), ('y', np.float64), ('z', np.float64)])
a = readrows(filename, dt, delimiter=';', sci='D', decimal=',')
print "a.shape =", a.shape
print "a.dtype =", a.dtype
print "a:"
print a
print
