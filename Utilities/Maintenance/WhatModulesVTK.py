#!/usr/bin/env python
import os, sys
import re

if len(sys.argv) != 3:
    print """
Usage: WhatModulesVTK.py vtkSourceTree applicationFile|applicationFolder
  Generate a FindPackage(VTK COMPONENTS) that lists all modules
    referenced by a set of files.
  Additionally, two extra find_package( VTK COMPONENTS) lists of modules
  are produced. One is a minimal set and the other chases down all the
  dependencies to produce a maximal set of modules. This is done by
  parsing the module.cmake files.

    For example:
      Running from the VTK source,
        ./Utilities/Maintenance/WhatModulesVTK.py . Filters/Modeling/Testing/Cxx/TestRotationalExtrusion.cxx
      Produces
        Modules and their dependencies:
        find_package(VTK COMPONENTS
          vtkCommonComputationalGeometry
          vtkCommonCore
          vtkCommonDataModel
          vtkCommonExecutionModel
          vtkCommonMath
          vtkCommonMisc
          vtkCommonSystem
          vtkCommonTransforms
          vtkFiltersCore
          vtkFiltersGeneral
          vtkFiltersModeling
          vtkFiltersSources
          vtkImagingCore
          vtkRenderingCore
          vtkRenderingOpenGL
          vtkTestingCore
          vtkTestingRendering
        )
        Your application code includes 17 of 170 vtk modules.

        All modules referenced in the files:
        find_package(VTK COMPONENTS
          vtkCommonCore
          vtkFiltersCore
          vtkFiltersModeling
          vtkFiltersSources
          vtkRenderingCore
          vtkRenderingOpenGL
          vtkTestingCore
          vtkTestingRendering
        )
        Your application code includes 8 of 170 vtk modules.

        Minimal set of modules:
        find_package(VTK COMPONENTS
          vtkCommonCore
          vtkFiltersCore
          vtkFiltersModeling
          vtkRenderingOpenGL
          vtkTestingRendering
        )
        Your application code includes 5 of 170 vtk modules.

"""
    exit(0)

def IncludesToPaths(path):
    '''
    Build a dict that maps include files to paths.
    '''
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

def FindModules(path):
    '''
    Build a dict that maps paths to modules.
    '''
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

def FindIncludes(path):
    '''
    Build a set that contains vtk includes.
    '''
    includes = set()
    includeProg = re.compile(r"(vtk.*\.h)")
    fid = open(path, "r")
    contents = fid.read()
    incs = includeProg.findall(contents)
    includes.update(incs)
    fid.close()
    return includes

def FindModuleFiles(path):
    '''
    Get a list of module files in the VTK directory.
    '''
    moduleFiles = [os.path.join(root, name)
                 for root, dirs, files in os.walk(path)
                 for name in files
                 if name == ("module.cmake")]
    return moduleFiles

def ParseModuleFile(fileName):
    '''
    Read each module file returning the module name and what
    it depends on or implements.
    '''
    fh = open(fileName, 'rb')
    lines = []
    for line in fh:
        line = line.strip()
        if line.startswith('$'): # Skip CMake variable names
            continue
        if line.startswith('#'):
            continue
        line = line.split('#')[0].strip() # inline comments
        if line == "":
            continue
        line = line.split(')')[0].strip() # closing brace with no space
        if line == "":
            continue
        for l in line.split(" "):
            lines.append(l)
    languages = ['PYTHON', 'TCL', 'JAVA']
    keywords = ['BACKEND', 'COMPILE_DEPENDS', 'DEPENDS', 'EXCLUDE_FROM_ALL',
                'EXCLUDE_FROM_WRAPPING', 'GROUPS', 'IMPLEMENTS', 'KIT',
                'PRIVATE_DEPENDS', 'TEST_DEPENDS'] + \
               map(lambda l: 'EXCLUDE_FROM_%s_WRAPPING' % l, languages)
    moduleName = ""
    depends = []
    implements = []
    state = "START";
    for item in lines:
        if state == "START" and item.startswith("vtk_module("):
            moduleName = item.split("(")[1]
            continue
        if item in keywords:
            state = item
            continue
        if state == 'DEPENDS' and item !=  ')':
            depends.append(item)
            continue
        if state == 'IMPLEMENTS' and item !=  ')':
            implements.append(item)
            continue
    return [moduleName, depends + implements]

def FindMinimalSetOfModules(modules, moduleDepencencies):
    '''
    Find the minimal set of modules needed.
    '''
    dependencies = set()
    for m in modules:
        dependencies = dependencies | set(moduleDepencencies[m]) # Set union
    return modules - dependencies # Set difference


def FindAllNeededModules(modules, foundModules, moduleDepencencies):
    '''
    Recursively search moduleDependencies finding all modules.
    '''
    if modules != None and len(modules) > 0:
        for m in modules:
            foundModules.add(m)
            foundModules = foundModules | set(moduleDepencencies[m]) # Set union
            foundModules = FindAllNeededModules(moduleDepencencies[m],
                                                foundModules,moduleDepencencies)
    return foundModules

def MakeFindPackage(modules):
    '''
    Make a useful find_package command.
    '''
    # Print a useful cmake command
    res = "find_package(VTK COMPONENTS\n"
    for module in sorted(modules):
        res +=  "  " + module + "\n"
    res +=  ")"
    return res

def main():
    '''
    Start the program
    '''
    # Generate dict's for mapping includes to modules
    includesToPaths = IncludesToPaths(sys.argv[1] + "/")
    pathsToModules = FindModules(sys.argv[1] + "/")

    # Test to see if VTK source is provided
    if len(pathsToModules) == 0:
        print sys.argv[1] +\
        " is not a VTK source directory. It does not contain any module.cmake files."
        exit(1)

    # Parse the module files making a dictionary of each module and its
    # dependencies or what it implements.
    moduleDepencencies = dict()
    moduleFiles = FindModuleFiles(sys.argv[1] + "/")
    for fname in moduleFiles:
        m = ParseModuleFile(fname)
        moduleDepencencies[m[0]] = m[1]

    # Build a set of includes for all command line files
    allIncludes = set()
    program = sys.argv[0]
    sys.argv.pop(0) # remove program name
    sys.argv.pop(0) # remove vtk source tree
    for f in sys.argv:
        if os.path.isfile(f):
            allIncludes.update(FindIncludes(f))
        else:
            # We have a folder so look through all the files.
            for path, dirs, files in os.walk(f):
                for fn in files:
                    allIncludes.update(FindIncludes(os.path.join(path,fn)))
    if len(allIncludes) == 0:
        print program + ": " + f + " does not exist"
        exit(1)

    # Build a set that contains all modules referenced in command line files
    allModules = set()
    for inc in allIncludes:
        if inc in includesToPaths:
            module = includesToPaths[inc]
            if module in pathsToModules:
                allModules.add(pathsToModules[includesToPaths[inc]])

    # Add vtkInteractionStyle if required.
    if "vtkRenderWindowInteractor.h" in allIncludes:
        allModules.add("vtkInteractionStyle")

    # Add OpenGL factory classes if required.
    if "vtkRenderingFreeType" in allModules:
        allModules.add("vtkRenderingFreeTypeFontConfig")
        allModules.add("vtkRenderingFreeTypeOpenGL")
    if "vtkRenderingCore" in allModules:
        allModules.add("vtkRenderingOpenGL")
    if "vtkRenderingVolume" in allModules:
        allModules.add("vtkRenderingVolumeOpenGL")

    # Find the minimal set of modules.
    minimalSetOfModules =\
        FindMinimalSetOfModules(allModules, moduleDepencencies)
    # Find all the modules, chasing down all the dependencies.
    allNeededModules =\
        FindAllNeededModules(minimalSetOfModules, set(), moduleDepencencies)

    modules = {'All modules referenced in the files:': allModules,
                'Minimal set of modules:': minimalSetOfModules,
                'Modules and their dependencies:': allNeededModules
              }
    for k, v in modules.iteritems():
        print k
        print MakeFindPackage(v)
        print "Your application code includes " + str(len(v)) +\
              " of " + str(len(pathsToModules)) + " vtk modules.\n"
    print

if __name__ == '__main__':
    main()
