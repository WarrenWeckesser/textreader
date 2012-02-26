from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

import numpy

ext = Extension("textreader",
                 [
                  "src/textreader.pyx",
                  "src/rows.c",
                  "src/tokenize.c",
                  "src/fields.c",
                  "src/file_buffer.c",
                  "src/conversions.c",
                  ],
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
