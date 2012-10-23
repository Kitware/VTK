#!/usr/bin/python

# imports
import re
import os, sys

# Create a dict that maps files of given suffix to their directories
def MapFilesToDirs(path, suffix):
    filesToDirs = dict()
    prog = re.compile(r".*."+suffix)
    for root, dirs, files in os.walk(path):
        for f in files:
            if prog.match(f):
                file = prog.findall(f)[0]
                parts = root.split("/")
                filesToDirs[f.rsplit(".",1)[0]] = root
    return filesToDirs

gcnoFilesToDirs = MapFilesToDirs(".", "gcno")
gcdaFilesToDirs = MapFilesToDirs(".", "gcda")

for f in gcdaFilesToDirs:
    src = gcdaFilesToDirs[f] + "/" + f + ".gcda"
    dst = gcnoFilesToDirs[f] + "/" + f + ".gcda"
    if (src != dst):
        os.rename(src, dst)
