
import sys
from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

import numpy

src_files = [
        "python/textreader.pyx",
        "src/rows.c",
        "src/tokenize.c",
        "src/fields.c",
        "src/conversions.c",
        "src/xstrtod.c",
        "src/str_to.c",
        ]


with_mmap = True
if with_mmap and (sys.platform == 'darwin' or sys.platform.startswith('linux')):
    print "Using memory mapped file buffer interface."
    src_files.append('src/file_buffer_mm.c')
else:
    src_files.append('src/file_buffer.c')

define_macros = []
if sys.platform.startswith('linux'):
    # XXX Is the condition for this too broad?  Should it be only for linux and gcc?
    define_macros.extend([('_FILE_OFFSET_BITS', '64'),
                          ('_XOPEN_SOURCE', '600')])

ext = Extension("textreader", src_files,
                include_dirs = ['src', numpy.get_include()],
                define_macros=define_macros)

setup(
    name='textreader',
    version='0.0.1',
    description='Read a text CSV (or similar) file into a NumPy array.',
    author='Warren Weckesser',
    author_email='warren.weckesser@enthought.com',
    license='BSD',
    ext_modules=[ext],
    cmdclass = {'build_ext': build_ext}
)
