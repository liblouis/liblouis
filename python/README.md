# Liblouis Python ctypes bindings

These bindings allow you to use liblouis from within Python. The
package is called "louis". See the documentation included in the
module for usage instructions.

These bindings use `ctypes` to access the liblouis shared library. The
liblouis shared library needs to be located in the library search
path. In most cases, if liblouis has been installed in a standard
location on your system, this is already the case and the bindings
will work without any additional steps.

A standard distutils `setup.py` script is provided for installation
tasks. To install this package for system wide use, after installing
`liblouis` itself, switch to the `python` directory and run (as root):

``` python
sudo python3 setup.py install
```

Some tests are located in `tests`. Note that these require that
you have compiled `liblouis` with support for 32-bit Unicode
characters.  See the top-level README for instructions on this option.

Remain in the `python` directory and run the tests with the
following command.  If successful, output will be brief, and note
that some tests are designed to have "expected failures".

``` console
sudo python3 tests/test_louis.py
```

## Python Virtual Environment

If you have a Python virtual environment created with the
Python `venv` module, follow these instructions to make
the `louis` module available in your virtual environment.

*  If your Python is at version 3.12 or newer, the `distutils`
module is deprecated.  You can make it available via a PIP
package with a command in your virtual environment like
```console
pip3 install setuptools
```
*  First, make yourself root or super-user with an
`su` or `sudo -s` command.
*  Then, activate your virtual environment.
*  Now do as above, but without `sudo`, since you are
already root.
```console
python3 setup.py install
```
*  You can now run the tests, as above, from within
your virtual environemnt.
