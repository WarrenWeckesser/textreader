
import numpy as np
from textreader import readrows


filename = 'data/embedded_newlines.csv'

dt = np.dtype([('x', np.int16), ('y', np.int16), ('notes', 'S50')])
data = readrows(filename, dt, delimiter=',')
print "data.shape =", data.shape
print "data.dtype =", data.dtype
print "data:"
print data
