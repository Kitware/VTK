"""Selective implementation of tcl info command.
This script is used while running python tests translated from Tcl."""

import re

def command (caller_globals, caller_locals, pattern):
    return commands(caller_globals, caller_locals, pattern)

def commands (caller_globals, caller_locals, pattern):
    print("pattern %s" % pattern)
    rex = re.compile(pattern)

    str = ""
    for c in caller_globals:
        if rex.match(c):
            str += c + " "
    for c in caller_locals:
        if rex.match(c):
            str += c + " "
    return str.strip()
