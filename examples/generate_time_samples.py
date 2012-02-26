
import time
import random


filename = 'data/time_samples.csv'
f = open(filename, 'w')
t0 = 1310220000.0
n = 50000
z1 = 12.5
z2 = 100.0
for k in range(n):
    t = t0 + 15 * k
    s = time.localtime(t)
    ft = time.strftime("%Y-%m-%d %H:%M:%S", s)
    print >> f, "{},{:8.3f},{:8.3f}".format(ft, z1, z2)
    z1 += random.gauss(0, 0.1)
    z2 += random.gauss(0, 0.12)
f.close()
