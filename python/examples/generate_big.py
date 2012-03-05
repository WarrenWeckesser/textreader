
import sys
import numpy as np


nrows = 2000000
ncols = 50
filename = 'data/big.csv'
f = open(filename, 'w')
print "Generating {}".format(filename)
for k in range(nrows):
    b = 0.25 * k
    values = np.linspace(b, b + 0.25, ncols, endpoint=False)
    s = ','.join(('%.3f' % x) for x in values) + '\n'
    f.write(s)
    q, r = divmod(100*(k+1), nrows)
    if r == 0:
        print "\r{:3d}%".format(q),
        sys.stdout.flush()
print
f.close()
