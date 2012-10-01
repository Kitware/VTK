#! /bin/env python

"""
This script will return the paths that distutils will use for installing
a package.  To use this script, execute it the same way that you would
execute setup.py, but instead of providing 'install' or 'build' as the
command, specify 'purelib' or 'platlib' and the corresponding path
will be printed.  The 'purelib' command will print the install location
for .py files, while the 'platlib' command will print the install location
of binary modules (.so or .dll).

Written by David Gobbi, Feb 25, 2006.
"""


import string
import sys
import os

def get_install_path(command, *args):
    """Return the module install path, given the arguments that were
    provided to setup.py.  The paths that you can request are 'purelib'
    for the .py installation directory and 'platlib' for the binary
    module installation directory.
    """

    # convert setup args into an option dictionary
    options = {}

    for arg in args:
        if arg == '--':
            break
        if arg[0:2] == "--":
            try:
                option, value = string.split(arg,"=")
                options[option] = value
            except ValueError:
                options[arg] = 1

    # check for the prefix and exec_prefix
    try:
        prefix = options["--prefix"]
    except KeyError:
        prefix = None

    try:
        exec_prefix = options["--exec-prefix"]
    except KeyError:
        exec_prefix = prefix

    # if prefix or exec_prefix aren't set, use default system values
    if prefix == None:
        prefix = sys.prefix

    if exec_prefix == None:
        exec_prefix = sys.exec_prefix

    # replace backslashes with slashes
    if os.name != 'posix':
        prefix = string.replace(prefix, os.sep, "/")
        exec_prefix = string.replace(exec_prefix, os.sep, "/")

    # get rid of trailing separator
    if prefix != "" and prefix[-1] == "/":
        prefix = prefix[0:-1]

    if exec_prefix != "" and exec_prefix[-1] == "/":
        exec_prefix = exec_prefix[0:-1]

    # check for "home" install scheme
    try:
        home = options["--home"]
        if os.name != 'posix':
            home = string.replace(home, os.sep, "/")
        if home != "" and home[-1] == "/":
            home = home[0:-1]
    except KeyError:
        home = None

    # apply "home" install scheme, but not for Windows with python < 2.4
    # (distutils didn't allow home scheme for windows until 2.4)
    if home != None and not (os.name != 'posix' and sys.version < '2.4'):
        purelib = home+'/lib/python'
        platlib = home+'/lib/python'
        scripts = home+'/bin'
        data    = home
    elif os.name == 'posix':
        ver = sys.version[0:3]
        purelib = prefix+'/lib/python'+ver+'/site-packages'
        platlib = exec_prefix+'/lib/python'+ver+'/site-packages'
        scripts = prefix+'/bin'
        data    = prefix
    elif sys.version < '2.2':
        purelib = prefix
        platlib = prefix
        scripts = prefix+'/Scripts'
        data    = prefix
    else:
        purelib = prefix+'/Lib/site-packages'
        platlib = prefix+'/Lib/site-packages'
        scripts = prefix+'/Scripts'
        data    = prefix

    # allow direct setting of install directories
    try:
        purelib = options["--install-purelib"]
    except KeyError:
        pass

    try:
        platlib = options["--install-platlib"]
    except KeyError:
        pass

    try:
        scripts = options["--install-scripts"]
    except KeyError:
        pass

    try:
        data = options["--install-data"]
    except KeyError:
        pass

    # return the information that was asked for
    if command == 'purelib':
        return purelib
    elif command == 'platlib':
        return platlib
    elif command == 'scripts':
        return scripts
    elif command == 'data':
        return data


if __name__ == "__main__":
    print apply(get_install_path, sys.argv[1:])
