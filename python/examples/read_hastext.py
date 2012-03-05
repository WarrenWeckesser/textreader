
import numpy as np
from textreader import readrows


filename = 'data/hastext.csv'

dt = np.dtype([('name', 'S12'), ('b', np.float32), ('c', np.int16)])
foo = readrows(filename, dt, delimiter=',')
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo
print

cols = (0, 2)
print "Now try usecols={}".format(cols)
print

dt = np.dtype([('name', 'S12'), ('c', np.int16)])
foo = readrows(filename, dt, delimiter=',', usecols=cols)
print "foo.shape =", foo.shape
print "foo.dtype =", foo.dtype
print "foo:"
print foo
