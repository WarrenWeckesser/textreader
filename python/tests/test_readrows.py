
from datetime import datetime
import os
import numpy as np
from numpy.testing import assert_array_equal, assert_equal
from textreader import readrows


filename = 'tmp.txt'


def my_assert_array_equal(a, b):
    if np.__version__ == '1.4.1' or np.__version__ == '1.5.1':
        # XXX Test fixed for 1.4.1 and 1.5.1. I haven't tracked down why
        # assert_array_equal fails in these older numpy versions.
        assert_equal(a.shape, b.shape)
        for arow, brow in zip(a, b):
            assert_array_equal(arow, brow)
    else:
        assert_array_equal(a, b)


def test1():
    dt = np.dtype([('x', float), ('y', float)])
    a = np.array([(1.0, 2.0), (3.0, 4.5)], dtype=dt)
    for sep in ' ,|':
        for fmt in ['%.18e', '%.2f', '%g']:
            np.savetxt(filename, a, delimiter=sep, fmt=fmt)
            b = readrows(filename, dt, delimiter=sep)
            assert_array_equal(a, b)
    os.remove(filename)


def test2():
    nrows = 100
    ncols = 50
    dt = np.dtype([('x' + str(k), np.float64) for k in range(ncols)])
    a = np.arange(float(nrows * ncols)).reshape(nrows, ncols).view(dt).squeeze()
    for sep in ' ,|':
        for fmt in ['%.18e', '%.2f']:
            np.savetxt(filename, a, delimiter=sep, fmt=fmt)
            b = readrows(filename, dt, delimiter=sep)
            assert_array_equal(a, b)
    os.remove(filename)


def test3():
    """Tests datetime_fmt and usecols."""
    text = """\
2011-01-02 00:30,1.0,,15,FR
2011-01-02 00:45,1.25,,16,TG
2011-01-02 00:56,1.5,,17,NK
2011-01-02 01:13,1.0,,18,QQ
"""

    f = open(filename, 'w')
    f.write(text)
    f.close()

    dt = np.dtype([('timestamp', np.datetime64), ('x', np.float32), ('index', np.int16), ('code', 'S2')])
    a = np.array([
        (datetime(2011, 1, 2, 0, 30, 0, 0), 1.0, 15, 'FR'),
        (datetime(2011, 1, 2, 0, 45, 0, 0), 1.25, 16, 'TG'),
        (datetime(2011, 1, 2, 0, 56, 0, 0), 1.5, 17, 'NK'),
        (datetime(2011, 1, 2, 1, 13, 0, 0), 1.0, 18, 'QQ')], dtype=dt)
    b = readrows(filename, dt, delimiter=',', usecols=(0, 1, -2, -1),
                 datetime_fmt="%Y-%m-%d %H:%M", tzoffset=None)
    assert_array_equal(a, b)
    os.remove(filename)


def test4():
    text = """\
  1.0,  1,  2,  3,  4
 10.0, 11, 12, 13, 14
100.0, 21, 22, 23, 24
"""

    f = open(filename, 'w')
    f.write(text)
    f.close()

    dt = np.dtype([('x', np.float32), ('codes', np.uint8, 4)])
    a = np.array([
        (1.0, (1, 2, 3, 4)),
        (10.0, (11, 12, 13, 14)),
        (100.0, (21, 22, 23, 24))], dtype=dt)
    b = readrows(filename, dt, delimiter=',')

    my_assert_array_equal(a, b)

    os.remove(filename)


def test5():
    text = """\
  1.0-2.0j, 3+4j
  5.0e-1, 6.0+0j
"""

    f = open(filename, 'w')
    f.write(text)
    f.close()

    dt = np.dtype([('x', np.complex128), ('y', np.complex128)])
    a = np.array([(1.0-2.0j, 3.0+4.0j), (0.5, 6.0)], dtype=dt)
    b = readrows(filename, dt, delimiter=',')
    my_assert_array_equal(a, b)

    os.remove(filename)


def test6():
    text = """\
  1.0,  1,  2,  3,  4
 10.0, 11, 12, 13, 14
100.0, 21, 22, 23, 24
"""

    f = open(filename, 'w')
    f.write(text)
    f.close()

    f = open(filename, 'r')

    dt = np.dtype([('x', np.float32), ('codes', np.uint8, 4)])
    a = np.array([(1.0, (1, 2, 3, 4))], dtype=dt)
    b = readrows(f, dt, delimiter=',', numrows=1)
    my_assert_array_equal(a, b)

    a = np.array([
        (10.0, (11, 12, 13, 14)),
        (100.0, (21, 22, 23, 24))], dtype=dt)
    b = readrows(f, dt, delimiter=',', numrows=2)
    my_assert_array_equal(a, b)

    f.close()
    os.remove(filename)
