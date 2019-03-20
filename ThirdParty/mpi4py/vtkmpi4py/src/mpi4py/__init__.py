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

__version__ = '3.0.1'
__author__ = 'Lisandro Dalcin'
__credits__ = 'MPI Forum, MPICH Team, Open MPI Team'


__all__ = ['MPI']


def get_include():
    """Return the directory in the package that contains header files.

    Extension modules that need to compile against mpi4py should use
    this function to locate the appropriate include directory. Using
    Python distutils (or perhaps NumPy distutils)::

      import mpi4py
      Extension('extension_name', ...
                include_dirs=[..., mpi4py.get_include()])

    """
    from os.path import join, dirname
    return join(dirname(__file__), 'include')


def get_config():
    """Return a dictionary with information about MPI."""
    from os.path import join, dirname
    try:
        from configparser import ConfigParser
    except ImportError:  # pragma: no cover
        from ConfigParser import ConfigParser
    parser = ConfigParser()
    parser.read(join(dirname(__file__), 'mpi.cfg'))
    return dict(parser.items('mpi'))


def rc(**kargs):  # pylint: disable=invalid-name
    """Runtime configuration options.

    Parameters
    ----------
    initialize : bool
        Automatic MPI initialization at import (default: True).
    threads : bool
        Request for thread support (default: True).
    thread_level : {'multiple', 'serialized', 'funneled', 'single'}
        Level of thread support to request (default: 'multiple').
    finalize : None or bool
        Automatic MPI finalization at exit (default: None).
    fast_reduce : bool
        Use tree-based reductions for objects (default: True).
    recv_mprobe : bool
        Use matched probes to receive objects (default: True).
    errors : {'exception', 'default', 'fatal'}
        Error handling policy (default: 'exception').

    """
    for key in kargs:
        if not hasattr(rc, key):
            raise TypeError("unexpected argument '{0}'".format(key))
    for key, value in kargs.items():
        setattr(rc, key, value)

rc.initialize = True
rc.threads = True
rc.thread_level = 'multiple'
rc.finalize = None
rc.fast_reduce = True
rc.recv_mprobe = True
rc.errors = 'exception'
__import__('sys').modules[__name__ + '.rc'] = rc


def profile(name, **kargs):
    raise RuntimeError('VTK\'s mpi4py does not support profiling')

profile.registry = []
