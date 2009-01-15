r"""Python bindings for liblouis

"""
import sys, os
from distutils.core import setup, Command, DistutilsOptionError
import louis

classifiers = [
    'Development Status :: 4 - Beta',
    'Intended Audience :: Developers',
    'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
    'Programming Language :: Python',
    'Topic :: Text Processing :: Linguistic',
    ]

setup(name="louis",
      description="Python MSAA client library",
      long_description = __doc__,
      download_url = "http://code.google.com/p/liblouis/",
      license="LGPLv2.2",
      classifiers=classifiers,
      version=louis.version().split(',')[0].split('-',1)[-1],
      packages=["louis"])
