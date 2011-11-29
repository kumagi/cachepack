#!/usr/bin/env python
# setup.py
from distutils.core import setup, Extension

module = Extension(
  '_pycached_impl',
  sources = ['src/pycached_impl.cc'],
  extra_compile_args = ['-O4'],
  include_dirs = ['/usr/local/include/boost'],
  library_dirs = ['/usr/local/lib'],
  libraries = ['boost_python']
)

setup(
    name = '_pycached_impl',
    version = '0.01',
    ext_modules = [module],
)

setup(
    name = 'cachepack',
    version = '0.01',
    package_dir={'': 'src'},
    py_modules = ['cachepack'],
)
