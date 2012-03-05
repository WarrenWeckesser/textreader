
import numpy as np
from textreader import readrows


filename = 'data/complex.dat'

f = open(filename, 'r')
s = f.read()
f.close()
print "File contents between lines:"
print "-"*25
print s,
print "-"*25
print

print "numpy.complex64:"
dt = np.dtype([('a', np.complex64), ('b', np.complex64)])
data = readrows(filename, dt)
print "data.shape =", data.shape
print "data.dtype =", data.dtype
print "data:"
print data
print

"""
print "numpy.complex128:"
dt = np.dtype([('a', np.complex128), ('b', np.complex128)])
data = readrows(filename, dt)
print "data.shape =", data.shape
print "data.dtype =", data.dtype
print "data:"
print data
print
"""