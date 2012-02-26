
import numpy
cimport numpy

cdef extern from "Python.h":
    ctypedef struct FILE
    FILE* PyFile_AsFile(object)

# Enter the builtin file class into the namespace:
cdef extern from "fileobject.h":
    ctypedef class __builtin__.file [object PyFileObject]:
        pass

cdef extern from "rows.h":
    int count_rows(FILE *f, char delimiter, char quote, char comment,
                   int allow_embedded_newline)
    void *read_rows(FILE *f, int nrows, char *fmt,
                    char delimiter, char quote, char comment,
                    int allow_embedded_newline,
                    char *datetime_fmt,
                    void *usecols, int num_usecols,
                    void *data_array)


def countrows(file f, delimiter=None, quote='"', comment='#',
                    allow_embedded_newline=True):
    cdef int count
    if delimiter is None:
        delimiter = ' '

    count = count_rows(PyFile_AsFile(f), ord(delimiter[0]), ord(quote[0]), ord(comment[0]),
                       allow_embedded_newline)
    return count


def dtypestr2fmt(st):
    if st == "f8":
        fmt = "d"
    elif st == "f4":
        fmt = "f"
    elif st == "i4":
        fmt = "i"
    elif st == "i2":
        fmt = "h"
    elif st == "M8[us]":
        fmt = "U"  # Temporary experiment
    elif st.startswith('S'):
        fmt = st[1:] + 's'
    else:
        raise ValueError('dtypestr2fmt: unsupported dtype string: %s' % (st,))
    return fmt


def _prod(x, y):
    return x*y


def dtype2fmt(dtype):
    """ This is not a finished, but it is sufficient for simple cases."""
    fmt = ''
    for name in dtype.names:
        field_dt = dtype[name]
        if field_dt.subdtype is not None:
            subdt, shape = field_dt.subdtype
            n = reduce(_prod, shape)
            subfmt = dtypestr2fmt(subdt.str[1:])
            fmt += subfmt * n
        else:
            fmt += dtypestr2fmt(field_dt.str[1:])
    return fmt


def readrows(f, dtype, delimiter=None, quote='"', comment='#',
             allow_embedded_newline=True, datetime_fmt=None,
             usecols=None):
    """
    Parameters
    ----------
    f : file or str
        File or name of file to read.
    dtype : numpy dtype
        Numpy dtype of the data to read.
        XXX Eventually this will be optional, and the code will figure
            it out automatically if the dtype is not given.
    quote : str with length of 1, optional
        Default is '"'.
    comment : str with length of 1, optional
        Default is '#'
    allow_embedded_newline : bool, optional
        Default is True
    datetime_fmt : str or None, optional
        If not None, this must be a string that can be used by
        strptime to parse a datetime string.
    usecols : sequence of ints, optional
        If given, this is the set of column indices (starting
        at 0) of the columns to keep.  The data type given in
        `dtype` must match the columns specified with `usecols`.
    """
    cdef numpy.ndarray a
    cdef numpy.ndarray usecols_array
    cdef char *dt_fmt
    cdef int opened_here = False

    if isinstance(f, basestring):
        opened_here = True
        filename = f
        f = open(f, 'r')

    fmt = dtype2fmt(dtype)

    if datetime_fmt is None:
        dt_fmt = ''
    else:
        dt_fmt = datetime_fmt

    if delimiter is None:
        delimiter = '\x00'

    nrows = countrows(f, delimiter, quote, comment, allow_embedded_newline)

    # XXX Remove this print eventually.  For now, it helps to show
    # how long countrows() takes.
    print "readrows: nrows =", nrows

    if usecols is None:
        usecols_array = numpy.arange(len(dtype.names), dtype=int)
    else:
        usecols_array = numpy.asarray(usecols, dtype=int)

    a = numpy.empty((nrows,), dtype=dtype)

    result = read_rows(PyFile_AsFile(f), nrows, fmt, ord(delimiter[0]), ord(quote[0]),
                         ord(comment[0]), allow_embedded_newline, dt_fmt,
                         <int *>usecols_array.data, usecols_array.size, a.data)

    if opened_here:
        f.close()

    return a
