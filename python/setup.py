# Liblouis Python ctypes bindings
#
#  Copyright (C) 2009 Eitan Isaacson <eitan@ascender.com>
#  Copyright (C) 2010 James Teh <jamie@jantrid.net>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., Franklin Street, Fifth Floor,
# Boston MA  02110-1301 USA.

r"""Python bindings for liblouis
"""

from distutils.core import setup
import louis

classifiers = [
    'Development Status :: 4 - Beta',
    'Intended Audience :: Developers',
    'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
    'Programming Language :: Python',
    'Topic :: Text Processing :: Linguistic',
    ]

setup(name="louis",
      description=__doc__,
      download_url = "http://code.google.com/p/liblouis/",
      license="LGPLv2.2",
      classifiers=classifiers,
      version=louis.version().split(',')[0].split('-',1)[-1],
      packages=["louis"])
