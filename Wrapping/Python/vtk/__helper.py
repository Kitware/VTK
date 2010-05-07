""" This provides some useful code used by other modules.  This is not to be
used by the end user which is why it is hidden. """

import string, sys
class LinkError(Exception):
    pass

def refine_import_err(mod_name, extension_name, exc):
    """ Checks to see if the ImportError was because the library
    itself was not there or because there was a link error.  If there
    was a link error it raises a LinkError if not it does nothing.

    Keyword arguments
    -----------------

     - mod_name : The name of the Python module that was imported.

     - extension_name : The name of the extension module that is to be
     imported by the module having mod_name.

     - exc : The exception raised when the module called mod_name was
     imported.

    To see example usage look at __init__.py.

    """
    try:
        del sys.modules['vtk.%s'%mod_name]
    except KeyError:
        pass
    if string.find(str(exc), extension_name) == -1:
	raise LinkError, str(exc)

