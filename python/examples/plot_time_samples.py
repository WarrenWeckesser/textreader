
import numpy as np
import os
import sys
import matplotlib.pyplot as plt
from textreader import readrows


filename = 'data/time_samples.csv'
if not os.path.exists(filename):
    print "Run the script generate_time_samples.py to generate the file {name}.".format(name=filename)
    sys.exit(0)

dt = np.dtype([('timestamp', np.datetime64), ('x', np.float32), ('y', np.float32)])

data = readrows(filename, delimiter=',', dtype=dt)

print "Read {name} into the array 'data'.".format(name=filename)
print "data.dtype =", data.dtype
print "data.shape =", data.shape
print "data[0] =", data[0]

dates = data['timestamp'].astype(object)

fig = plt.figure()
ax1 = plt.subplot(2,1,1)
plt.plot(dates, data['x'])
plt.ylabel('x')
fig.autofmt_xdate()
plt.grid()
plt.subplot(2,1,2, sharex=ax1)
plt.plot(dates, data['y'])
plt.ylabel('y')
fig.autofmt_xdate()
plt.grid()
plt.show()
