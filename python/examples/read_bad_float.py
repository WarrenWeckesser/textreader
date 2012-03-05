
import numpy as np
from textreader import readrows


filename = 'data/bad_float.csv'

dt = np.dtype([('a', np.int16), ('b', np.int16), ('x', np.float32)])
foo = readrows(filename, dt, delimiter=',')
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo
print
