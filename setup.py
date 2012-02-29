
import sys
from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

import numpy

src_files = [
        "src/textreader.pyx",
        "src/rows.c",
        "src/tokenize.c",
        "src/fields.c",
        "src/conversions.c",
        ]

if False and (sys.platform == 'darwin' or sys.platform.startswith('linux')):
    src_files.append('src/file_buffer_mm.c')
else:
    src_files.append('src/file_buffer.c')

ext = Extension("textreader", src_files,
                include_dirs = ['src', numpy.get_include()])

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
