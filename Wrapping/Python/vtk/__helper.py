""" This provides some useful code used by other modules.  This is not to be
used by the end user which is why it is hidden. """

import string, sys
class LinkError(Exception):
    pass

def refine_import_err(mod_name, exc):
    """ Checks to see if the ImportError was because the library
    itself was not there or because there was a link error.  If there
    was a link error it raises a LinkError if not it does nothing."""
    del sys.modules['vtk.%s'%mod_name]
    mod = 'vtk' + string.upper(mod_name[0]) + mod_name[1:] +'Python'
    if string.find(str(exc), mod) == -1:
	raise LinkError, str(exc)

# Python "help(sys.setdlopenflags)" states:
#
# setdlopenflags(...)
#     setdlopenflags(n) -> None
#     
#     Set the flags that will be used for dlopen() calls. Among other
#     things, this will enable a lazy resolving of symbols when
#     importing a module, if called as sys.setdlopenflags(0) To share
#     symbols across extension modules, call as
#
#     sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)
#
# GCC 3.x depends on proper merging of symbols for RTTI:
#   http://gcc.gnu.org/faq.html#dso
#
try:
  import dl
  sys.setdlopenflags(dl.RTLD_NOW|dl.RTLD_GLOBAL)
except: pass
