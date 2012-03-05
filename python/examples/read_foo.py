
import numpy as np
from textreader import readrows


filename = 'data/foo.csv'

dt = np.dtype([('a', np.int16), ('b', np.int16), ('x', np.float32)])
foo = readrows(filename, dt, delimiter=',')
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo
print

dt = np.dtype([('a', np.int16, 2), ('x', np.float32)])
foo = readrows(filename, dt, delimiter=',')
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo
print

dt = np.dtype([('a', np.int16), ('b', np.int16), ('x', np.float32)])
print "loadtxt:"
foo = np.loadtxt(filename, dtype=dt, delimiter=',')
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo