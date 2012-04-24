"""selective implementation of tcl file command.
This script is used while running python tests translated from Tcl."""

import os
import stat

def is_dir(filename):
    if stat.S_ISDIR(os.stat(filename)[stat.ST_MODE]):
        return True
    return False

def delete( *arguments ):
    end_of_switch = False
    force_switch = False
    pathnames = []
    for arg in arguments:
        if arg.strip() == "-force" and not end_of_switch:
            force_switch = True
        elif arg.strip() == "--" and not end_of_switch:
            end_of_switch = True
        else:
            pathnames.append(arg)
    for filename in pathnames:
        #dir is deleted only if force is specified.
        try:
            if is_dir(filename):
                if force_switch:
                    os.rmdirs(filename)
            else:
                os.remove(filename)
        except:
            pass

