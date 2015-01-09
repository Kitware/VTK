# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
"""
This is the **MPI for Python** package.

What is *MPI*?
==============

The *Message Passing Interface*, is a standardized and portable
message-passing system designed to function on a wide variety of
parallel computers. The standard defines the syntax and semantics of
library routines and allows users to write portable programs in the
main scientific programming languages (Fortran, C, or C++). Since
its release, the MPI specification has become the leading standard
for message-passing libraries for parallel computers.

What is *MPI for Python*?
=========================

*MPI for Python* provides MPI bindings for the Python programming
language, allowing any Python program to exploit multiple processors.
This package is constructed on top of the MPI-1/2 specifications and
provides an object oriented interface which closely follows MPI-2 C++
bindings.
"""

__version__   = '1.3'
__author__    = 'Lisandro Dalcin'
__credits__   = 'MPI Forum, MPICH Team, Open MPI Team.'

# --------------------------------------------------------------------

__all__ = ['MPI', 'MPE']

# --------------------------------------------------------------------

def get_include():
    """
    Return the directory in the package that contains header files.

    Extension modules that need to compile against mpi4py should use
    this function to locate the appropriate include directory. Using
    Python distutils (or perhaps NumPy distutils)::

      import mpi4py
      Extension('extension_name', ...
                include_dirs=[..., mpi4py.get_include()])

    """
    from os.path import dirname, join
    return join(dirname(__file__), 'include')

# --------------------------------------------------------------------

def get_config():
    """
    Return a dictionary with information about MPI.
    """
    from os.path import dirname, join
    try:
        from configparser import ConfigParser
    except ImportError:
        from ConfigParser import ConfigParser
    parser = ConfigParser()
    parser.read(join(dirname(__file__), 'mpi.cfg'))
    return dict(parser.items('mpi'))

# --------------------------------------------------------------------
