#!/usr/bin/env python
# This is the script that runs a testing python script.
# The script to be run must be the first argument.
import sys
if len(sys.argv) < 2:
    print("Usage %s <test script> [<addition arguments>]" % sys.argv[0])
    sys.exit(1)
for i in range(2, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

import vtk

#these are the modules that define methods/variables
#used by many scripts. We just include them always
from backdrop import *
from mccases import *
import catch
import expr
import file
import info
import math
from vtk.util.colors import *


# Mock class that overrides the Start() method from vtkRenderWindowInteractor
# to do nothing. This allows VTK's python tests to be standard VTK scripts that
# call Start() on the interactor.
class vtkTestingInteractor(vtk.vtkRenderWindowInteractor):
    def Start(self):
        pass


#implementation for lindex.
def lindex(list, index):
    if type(list) == type("string"):
        return list.split()[index]
    return list[index]

#gets with no varName (returns the read string)
def gets(file):
    line = file.readline()
    if line[-1] == "\n":
        line = line[:-1]
    return line

def gets(file, varName, global_vars):
    line = gets(file)
    ret = len(line)
    global_vars[varName] = line
    return ret

def tcl_platform(what):
    if what != "platform":
        raise ValueError("Only platform supported as yet!")
    plat = sys.platform
    if plat[:5] == "linux":
        return "unix"
    return plat

def get_variable_name(*args):
    var_name = ""
    for arg in args:
        if arg == "":
            continue
        # it is essential to qualify the scope of type since
        # some test define type variable which messes up the
        # builtin call.
        if __builtins__.type(arg) == __builtins__.type("string"):
            var_name += arg
        else:
            var_name += repr(arg)
    return var_name

#init Tk
try:
    import Tkinter
    pythonTk = Tkinter.Tk()
    pythonTk.withdraw()
except:
    pythonTk = None
    pass #no hassles if Tk is not present.

# setup some common things for testing
rtTempObject = vtk.vtkObject()

rtExMath = vtk.vtkMath()
rtExMath.RandomSeed(6)

# create the testing class to do the work
rtTester = vtk.vtkTesting()
for arg in sys.argv[2:]:
    rtTester.AddArgument(arg)

VTK_DATA_ROOT = rtTester.GetDataRoot()

if rtTester.IsInteractiveModeSpecified() == 0:
    vtk.vtkRenderWindowInteractor = vtkTestingInteractor

# load in the script
test_script = sys.argv[1]

# set the default threshold, the Tcl script may change this
threshold = -1


# we pass the locals over so that the test script has access to
# all the locals we have defined here.
exec(compile(open(test_script).read(), test_script, 'exec'), globals(), locals())

local_variables_dict = locals()


if "iren" in local_variables_dict:
    renWin.Render()

if pythonTk:
    # run the event loop quickly to map any tkwidget windows
    pythonTk.update()

rtResult = 0

if rtTester.IsValidImageSpecified() != 0:
    # look for a renderWindow ImageWindow or ImageViewer
    # first check for some common names
    if "renWin" in local_variables_dict:
        rtTester.SetRenderWindow(renWin)
        if threshold == -1:
            threshold = 10
    else:
        if threshold == -1:
            threshold = 5

        if "viewer" in local_variables_dict:
            rtTester.SetRenderWindow(viewer.GetRenderWindow())
            viewer.Render()
        elif "imgWin" in local_variables_dict:
            rtTester.SetRenderWindow(imgWin)
            imgWin.Render()
    rtResult = rtTester.RegressionTest(threshold)

if rtResult == 0:
    sys.exit(1)
sys.exit(0)
