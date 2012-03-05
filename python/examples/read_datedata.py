
import numpy as np
import os
import sys
from textreader import readrows


filename = 'data/datedata.txt'

dt = np.dtype([('timestamp', np.datetime64), ('x', np.float32)])

data = readrows(filename, delimiter=',', dtype=dt, datetime_fmt="%m/%d/%Y %H:%M:%S")

print "Read {name} into the array 'data'.".format(name=filename)
print "data.dtype =", data.dtype
print "data.shape =", data.shape
print "data:"
print data
