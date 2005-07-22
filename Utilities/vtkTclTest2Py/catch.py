""" implementation for tcl catch.
since this call may need to create new vars in the caller scope
we have to use globals(), since locals are readonly.
This script is used while running python tests translated from Tcl."""

import sys
def catch(caller_globalvars_dict, code_to_catch):
    try:
        exec(code_to_catch, caller_globalvars_dict)
    except:
        print "caught exception: %s" % sys.exc_info()[1]
        return 1 #returns non-zero on exception
    return 0 #return zero when no exception is raised
