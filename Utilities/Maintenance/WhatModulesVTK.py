#!/usr/bin/env python
import os, sys
import re

if len(sys.argv) < 3:
    print """
Usage: WhatModulesVTK.py vtkSourceTree applicationFiles...
  Generate a FindPackage(VTK COMPONENTS) that lists all modules
    referenced by a set of files

    For example:
      Running from the VTK source,
        ./Utilities/Maintenance/WhatModulesVTK.py . Filters/Modeling/Testing/Cxx/TestRotationalExtrusion.cxx
      Produces
        Find_Package(VTK COMPONENTS
          vtkCommonCore
          vtkFiltersCore
          vtkFiltersModeling
          vtkFiltersSources
          vtkRenderingCore
          vtkTestingCore
          vtkTestingRendering
          vtkRenderingOpenGL
          )
       To select many files from an application,
         ./Utilities/Maintenance/WhatModulesVTK.py . $(find /path/to/vtk/project/ -type f)
"""
    exit(0)

# Build a dict that maps include files to paths
def IncludesToPaths(path):
    includeToPath = dict()
    prog = re.compile(r"(vtk.*\.h)")
    for root, dirs, files in os.walk(path):
        for f in files:
            if prog.match(f):
                includeFile = prog.findall(f)[0]
                parts = root.split("/")
                module = parts[len(parts)-2] + parts[len(parts)-1]
                includeToPath[includeFile] = module
    return includeToPath

# Build a dict that maps paths to modules
def FindModules(path):
    pathToModule = dict()
    fileProg = re.compile(r"module.cmake")
    moduleProg = re.compile(r".*module[^(]*\(\s*(\w*)",re.S)
    for root, dirs, files in os.walk(path):
        for f in files:
            if fileProg.match(f):
                fid = open(os.path.join(root, f), "r")
                contents = fid.read()
                m = moduleProg.match(contents)
                if m:
                    moduleName = m.group(1)
                    parts = root.split("/")
                    pathToModule[parts[len(parts)-2] + parts[len(parts)-1]] = moduleName
                fid.close()
    return pathToModule

# Build a set that contains vtk includes
def FindIncludes(path):
    includes = set()
    includeProg = re.compile(r"(vtk.*\.h)")
    fid = open(path, "r")
    contents = fid.read()
    incs = includeProg.findall(contents)
    includes.update(incs)
    fid.close()
    return includes

# Start the program

# Generate dict's for mapping includes to modules
includesToPaths = IncludesToPaths(sys.argv[1] + "/")
pathsToModules = FindModules(sys.argv[1] + "/")

# Test to see if VTK source is provided
if len(pathsToModules) == 0:
    print sys.argv[1] + " is not a VTK source directory. It does not contain any module.cmake files."
    exit(1)

# Build a set of includes for all command line files
allIncludes = set()
program = sys.argv[0]
sys.argv.pop(0) # remove program name
sys.argv.pop(0) # remove vtk source tree
for f in sys.argv:
    if os.path.isfile(f):
        allIncludes.update(FindIncludes(f))
    else:
        print program + ": " + f + " does not exist"
        exit(1)
# Build a set that contains all modules referenced in command line files
allModules = set()
for inc in allIncludes:
    if inc in includesToPaths:
        module = includesToPaths[inc]
        if module in pathsToModules:
            allModules.add(pathsToModules[includesToPaths[inc]])

# Add OpenGL factory classes if required
if "vtkRenderingFreeType" in allModules:
    allModules.add("vtkRenderingFreeTypeFontConfig")
    allModules.add("vtkRenderingFreeTypeOpenGL")
if "vtkRenderingCore" in allModules:
    allModules.add("vtkRenderingOpenGL")
if "vtkRenderingVolume" in allModules:
    allModules.add("vtkRenderingVolumeOpenGL")

# Print a useful cmake command
print "find_package(VTK COMPONENTS"
for module in sorted(allModules):
    print "  " + module


print ")"

print "Your application code includes " + str(len(allModules)) + " of " + str(len(pathsToModules)) + " vtk modules."
