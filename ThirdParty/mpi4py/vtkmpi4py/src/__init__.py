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

__version__ = '2.0.0'
__author__  = 'Lisandro Dalcin'
__credits__ = 'MPI Forum, MPICH Team, Open MPI Team'

# --------------------------------------------------------------------

__all__ = ['MPI']

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
    from os.path import join, dirname
    return join(dirname(__file__), 'include')

# --------------------------------------------------------------------


def get_config():
    """
    Return a dictionary with information about MPI.
    """
    from os.path import join, dirname
    try:
        # pylint: disable=import-error
        from configparser import ConfigParser
    except ImportError:
        # pylint: disable=import-error
        from ConfigParser import ConfigParser
    parser = ConfigParser()
    parser.read(join(dirname(__file__), 'mpi.cfg'))
    return dict(parser.items('mpi'))

# --------------------------------------------------------------------


def rc(**kargs):  # pylint: disable=invalid-name
    """
    Runtime configuration options.

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
            raise TypeError("unexpected argument '%s'" % key)
    for key, value in kargs.items():
        setattr(rc, key, value)

rc.initialize = True
rc.threads = True
rc.thread_level = 'multiple'
rc.finalize = None
rc.fast_reduce = True
rc.recv_mprobe = True
rc.errors = 'exception'

from sys import modules
modules[__name__ + '.rc'] = rc
del modules

# --------------------------------------------------------------------


def profile(name='mpe', **kargs):
    """
    Support for the MPI profiling interface.

    Parameters
    ----------
    name : str, optional
       Name of the profiler to load.
    path : list of str, optional
       Additional paths to search for the profiler.
    logfile : str, optional
       Filename prefix for dumping profiler output.
    """
    # pylint: disable=too-many-locals
    # pylint: disable=too-many-branches
    import sys
    import os
    try:
        from .dl import dlopen, dlerror, RTLD_NOW, RTLD_GLOBAL
    except ImportError:  # pragma: no cover
        from ctypes import CDLL as dlopen, RTLD_GLOBAL
        try:
            # pylint: disable=import-error
            from DLFCN import RTLD_NOW
        except ImportError:
            RTLD_NOW = 2  # pylint: disable=invalid-name
        dlerror = None

    def lookup_dylib(name, path):
        # pylint: disable=missing-docstring
        import imp
        pattern = []
        for suffix, _, kind in imp.get_suffixes():
            if kind == imp.C_EXTENSION:
                pattern.append(('', suffix))
        if sys.platform.startswith('win'):  # pragma: no cover
            pattern.append(('', '.dll'))
        elif sys.platform == 'darwin':  # pragma: no cover
            pattern.append(('lib', '.dylib'))
        elif os.name == 'posix':  # pragma: no cover
            pattern.append(('lib', '.so'))
        pattern.append(('', ''))
        for pth in path:
            for (lib, dso) in pattern:
                filename = os.path.join(pth, lib + name + dso)
                if os.path.isfile(filename):
                    return filename
        return None
    #
    logfile = kargs.pop('logfile', None)
    if logfile:
        if name in ('mpe',):
            if 'MPE_LOGFILE_PREFIX' not in os.environ:
                os.environ['MPE_LOGFILE_PREFIX'] = logfile
        if name in ('vt', 'vt-mpi', 'vt-hyb'):
            if 'VT_FILE_PREFIX' not in os.environ:
                os.environ['VT_FILE_PREFIX'] = logfile
    path = kargs.pop('path', None)
    if path is None:
        path = []
    elif isinstance(path, str):
        path = [path]
    else:
        path = list(path)

    prefix = os.path.dirname(__file__)
    path.append(os.path.join(prefix, 'lib-pmpi'))
    filename = lookup_dylib(name, path)
    if filename is None:
        raise ValueError("profiler '%s' not found" % name)
    else:
        filename = os.path.abspath(filename)

    handle = dlopen(filename, RTLD_NOW | RTLD_GLOBAL)
    if handle:  # pragma: no branch
        profile.registry.append((name, (handle, filename)))
    else:  # pragma: no cover
        from warnings import warn
        if dlerror:
            message = dlerror()
        else:
            message = "error loading '%s'" % filename
        warn(message)

profile.registry = []

# --------------------------------------------------------------------
