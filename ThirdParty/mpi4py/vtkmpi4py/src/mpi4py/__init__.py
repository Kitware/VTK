# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
"""The **MPI for Python** package.

The *Message Passing Interface* (MPI) is a standardized and portable
message-passing system designed to function on a wide variety of
parallel computers. The MPI standard defines the syntax and semantics
of library routines and allows users to write portable programs in the
main scientific programming languages (Fortran, C, or C++). Since its
release, the MPI specification has become the leading standard for
message-passing libraries for parallel computers.

*MPI for Python* provides MPI bindings for the Python programming
language, allowing any Python program to exploit multiple processors.
This package build on the MPI specification and provides an object
oriented interface which closely follows MPI-2 C++ bindings.

"""

__version__ = '4.0.1'
__author__ = 'Lisandro Dalcin'
__credits__ = 'MPI Forum, MPICH Team, Open MPI Team'


__all__ = ['MPI']


class Rc:
    """Runtime configuration options.

    Attributes
    ----------
    initialize : bool
        Automatic MPI initialization at import (default: True).
    threads : bool
        Request initialization with thread support (default: True).
    thread_level : {"multiple", "serialized", "funneled", "single"}
        Level of thread support to request (default: "multiple").
    finalize : None or bool
        Automatic MPI finalization at exit (default: None).
    fast_reduce : bool
        Use tree-based reductions for objects (default: True).
    recv_mprobe : bool
        Use matched probes to receive objects (default: True).
    irecv_bufsz : int
        Default buffer size in bytes for ``irecv()`` (default = 32768).
    errors : {"exception", "default", "abort", "fatal"}
        Error handling policy (default: "exception").

    """

    initialize = True
    threads = True
    thread_level = 'multiple'
    finalize = None
    fast_reduce = True
    recv_mprobe = True
    irecv_bufsz = 32768
    errors = 'exception'

    def __init__(self, **kwargs):
        """Initialize options."""
        self(**kwargs)

    def __setattr__(self, name, value):
        """Set option."""
        if not hasattr(self, name):
            raise TypeError(f"object has no attribute {name!r}")
        super().__setattr__(name, value)

    def __call__(self, **kwargs):
        """Update options."""
        for key in kwargs:
            if not hasattr(self, key):
                raise TypeError(f"unexpected argument {key!r}")
        for key, value in kwargs.items():
            setattr(self, key, value)

    def __repr__(self):
        """Return repr(self)."""
        return f'<{__spec__.name}.rc>'


rc = Rc()
__import__('sys').modules[__spec__.name + '.rc'] = rc


def get_include():
    """Return the directory in the package that contains header files.

    Extension modules that need to compile against mpi4py should use
    this function to locate the appropriate include directory. Using
    Python distutils (or perhaps NumPy distutils)::

      import mpi4py
      Extension('extension_name', ...
                include_dirs=[..., mpi4py.get_include()])

    """
    # pylint: disable=import-outside-toplevel
    from os.path import join, dirname
    return join(dirname(__spec__.origin), 'include')


def get_config():
    """Return a dictionary with information about MPI.

    .. versionchanged:: 4.0.0
       By default, this function returns an empty dictionary. However,
       downstream packagers and distributors may alter such behavior.
       To that end, MPI information must be provided under an ``mpi``
       section within a UTF-8 encoded INI-style configuration file
       :file:`mpi.cfg` located at the top-level package directory.
       The configuration file is read and parsed using the
       `configparser` module.

    """
    # pylint: disable=import-outside-toplevel
    from configparser import ConfigParser
    from os.path import join, dirname
    parser = ConfigParser()
    parser.add_section('mpi')
    mpicfg = join(dirname(__spec__.origin), 'mpi.cfg')
    parser.read(mpicfg, encoding='utf-8')
    return dict(parser.items('mpi'))


def profile(name, *, path=None):
    raise RuntimeError('VTK\'s mpi4py does not support profiling')
