#!/usr/bin/env python
# This is the script that runs Python regression test scripts.
# The test script to be run must be the first argument.

from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkTestingRendering import vtkTesting
import os
import sys
import re
import traceback

def main(test_script):
    """Run a regression test, and compare the contents of the window against
    against a valid image.  This will use arguments from sys.argv to set the
    testing options via the vtkTesting class, run the test script, and then
    call vtkTesting.RegressionTest() to validate the image.  The return
    value will the one provided by vtkTesting.RegressionTest().
    """

    # find the first argument that's an option
    opt1 = 1
    while opt1 < len(sys.argv) and not sys.argv[opt1].startswith('-'):
        opt1 += 1

    # get the "-A" option, which isn't handled by vtkTester
    for i in range(opt1, len(sys.argv)):
        if sys.argv[i] == '-A' and i < len(sys.argv) - 1:
            sys.path.append(sys.argv[i + 1])

    # create the testing class to do the work
    rtTester = vtkTesting()
    for arg in sys.argv[opt1:]:
        rtTester.AddArgument(arg)

    # if test is not interactive, make a mock interactor with a
    # disabled Start method
    if rtTester.IsInteractiveModeSpecified() == 0:
        from vtkmodules.vtkRenderingCore import vtkRenderWindowInteractor
        import vtkmodules.vtkRenderingUI
        irenType = type(vtkRenderWindowInteractor())
        class vtkTestingInteractor(irenType):
            def Start(self):
                pass
        irenType.override(vtkTestingInteractor)

    # seed the random number generator
    vtkMath.RandomSeed(6)

    # read the test script
    with open(test_script) as test_file:
        test_code = test_file.read()

    # inject the test script's directory into sys.path
    test_script_dir = os.path.abspath(os.path.dirname(test_script))
    sys.path.insert(0, test_script_dir)

    # we provide an initial set of variables for the test script
    test_script_vars = { "__name__" : "__main__" }

    try:
        # run the test and capture all of its global variables
        exec(compile(test_code, test_script, 'exec'), test_script_vars)
    except Exception:
        traceback.print_exc()
        return vtkTesting.FAILED
    finally:
        # undo the change to the path
        sys.path.remove(test_script_dir)

    # undo the vtkRenderWindowInteractor override
    if rtTester.IsInteractiveModeSpecified() == 0:
       irenType.override(None)

    # if script has "if __name__ == '__main__':", then we assume that
    # it would have raised an exception or called exit(1) if it failed.
    if re.search('^if *__name__ *== *[\'\"]__main__[\'\"]', test_code, flags=re.MULTILINE):
        return vtkTesting.PASSED

    # if script didn't set "threshold", use default value
    try:
        threshold = test_script_vars["threshold"]
    except KeyError:
        threshold = 0.15

    # we require a valid regression image
    if rtTester.IsValidImageSpecified():
        # look for a renderWindow ImageWindow or ImageViewer
        # first check for some common names
        if "iren" in test_script_vars:
            iren = test_script_vars["iren"]
            rtTester.SetRenderWindow(iren.GetRenderWindow())
            iren.GetRenderWindow().Render()
        elif "renWin" in test_script_vars:
            renWin = test_script_vars["renWin"]
            rtTester.SetRenderWindow(renWin)
        elif "viewer" in test_script_vars:
            viewer = test_script_vars["viewer"]
            rtTester.SetRenderWindow(viewer.GetRenderWindow())
            viewer.Render()
        elif "imgWin" in test_script_vars:
            imgWin = test_script_vars["imgWin"]
            rtTester.SetRenderWindow(imgWin)
            imgWin.Render()
        else:
            sys.stderr.write("Test driver rtImageTest.py says \"no iren, renWin, viewer, or imgWin\": %s\n" % test_script)
            return vtkTesting.FAILED

        return rtTester.RegressionTest(threshold)

    return vtkTesting.FAILED

if __name__ == '__main__':
    # We don't parse the arguments (vtkTesting does that), but we need
    # to extract the name of the test script to run.
    if len(sys.argv) < 2 or sys.argv[1].startswith('-'):
        print("Usage %s <test script> [<addition arguments>]" % argv[0])
        sys.exit(1)

    if main(sys.argv[1]) == vtkTesting.FAILED:
        sys.exit(1)
