""" This module attempts to make it easy to create VTK-Python
unittests.  The module uses unittest for the test interface.  For more
documentation on what unittests are and how to use them, please read
these:

   http://www.python.org/doc/current/lib/module-unittest.html
   
   http://www.diveintopython.org/roman_divein.html


This VTK-Python test module supports image based tests with multiple
images per test suite and multiple images per individual test as well.
It also prints information appropriate for Dart
(http://public.kitware.com/Dart/).

This module defines several useful classes and functions to make
writing tests easy.  The most important of these are:

class vtkTest:
   Subclass this for your tests.  It also has a few useful internal
   functions that can be used to do some simple blackbox testing.

compareImage(renwin, img_fname, threshold=10):
   Compares renwin with image and generates image if it does not
   exist.  The threshold determines how closely the images must match.
   The function also handles multiple images and finds the best
   matching image.

getAbsImagePath(img_basename):
   Returns the full path to the image given the basic image name.

main(cases):
   Does the testing given a list of tuples containing test classes and
   the starting string of the functions used for testing.

interact():
    Interacts with the user if necessary.  The behavior of this is
    rather trivial and works best when using Tkinter.  It does not do
    anything by default and stops to interact with the user when given
    the appropriate command line arguments.

Examples:

  The best way to learn on how to use this module is to look at a few
  examples.  The end of this file contains a trivial example.  Please
  also look at the following examples:

    Rendering/Testing/Python/TestTkRenderWidget.py,
    Rendering/Testing/Python/TestTkRenderWindowInteractor.py

Created: September, 2002

Prabhu Ramachandran <prabhu@aero.iitm.ernet.in>
"""

import sys, os, time
import unittest, getopt
import vtk
import BlackBox

# location of the VTK data files.  Set via command line args or
# environment variable.
VTK_DATA_ROOT = ""

# location of the VTK baseline images.  Set via command line args or
# environment variable.
VTK_BASELINE_ROOT = ""

# Verbosity of the test messages (used by unittest)
_VERBOSE = 0

# Determines if it is necessary to interact with the user.  If zero
# dont interact if 1 interact.  Set via command line args
_INTERACT = 0

# This will be set to 1 when the image test will not be performed.
# This option is used internally by the script and set via command
# line arguments.
_NO_IMAGE = 0


class vtkTest(unittest.TestCase):
    """A simple default VTK test class that defines a few useful
    blackbox tests that can be readily used.  Derive your test cases
    from this class and use the following if you'd like to.

    Note: Unittest instantiates this class (or your subclass) each
    time it tests a method.  So if you do not want that to happen when
    generating VTK pipelines you should create the pipeline in the
    class definition as done below for _blackbox.
    """
    
    _blackbox = BlackBox.Tester(debug=0)

    def _testParse(self, obj):
        """Does a blackbox test by attempting to parse the class for
        its various methods using vtkMethodParser.  This is a useful
        test because it gets all the methods of the vtkObject, parses
        them and sorts them into different classes of objects."""
        self._blackbox.testParse(obj)

    def _testGetSet(self, obj):
        """Checks the Get/Set method pairs by setting the value using
        the current state and making sure that it equals the value it
        was originally.  This effectively calls _testParse
        internally. """
        self._blackbox.testGetSet(obj)

    def _testBoolean(self, obj):
        """Checks the Boolean methods by setting the value on and off
        and making sure that the GetMethod returns the the set value.
        This effectively calls _testParse internally. """
        self._blackbox.testBoolean(obj)
        


def interact():
    """Interacts with the user if necessary. """
    global _INTERACT
    if _INTERACT:
        raw_input("\nPress Enter/Return to continue with the testing. --> ")


def getAbsImagePath(img_basename):    
    """Returns the full path to the image given the basic image
    name."""
    global VTK_BASELINE_ROOT
    return os.path.join(VTK_BASELINE_ROOT, img_basename)


def compareImage(renwin, img_fname, threshold=10):
    """Compares renwin's (a vtkRenderWindow) contents with the image
    file whose name is given in the second argument.  If the image
    file does not exist the image is generated and stored.  If not the
    image in the render window is compared to that of the figure.
    This function also handles multiple images and finds the best
    matching image.  """

    global _NO_IMAGE
    if _NO_IMAGE:
        return
    
    f_base, f_ext = os.path.splitext(img_fname)

    w2if = vtk.vtkWindowToImageFilter()
    w2if.ReadFrontBufferOff()
    w2if.SetInput(renwin)

    if not os.path.isfile(img_fname):
        # generate the image
        pngw = vtk.vtkPNGWriter()
        pngw.SetFileName(img_fname)
        pngw.SetInput(w2if.GetOutput())
        pngw.Write()
        return 
        
    pngr = vtk.vtkPNGReader()
    pngr.SetFileName(img_fname)

    idiff = vtk.vtkImageDifference()
    idiff.SetInput(w2if.GetOutput())
    idiff.SetImage(pngr.GetOutput())
    idiff.Update()
    del w2if

    min_err = idiff.GetThresholdedError()
    img_err = min_err
    best_img = img_fname

    err_index = 0
    count = 0
    if min_err > threshold:
        count = 1
        test_failed = 1
        err_index = -1
        while 1: # keep trying images till we get the best match.
            new_fname = f_base + "_%d.png"%count
            if not os.path.exists(new_fname):
                # no other image exists.
                break
            # since file exists check if it matches.
            pngr.SetFileName(new_fname)
            pngr.Update()
            idiff.Update()
            alt_err = idiff.GetThresholdedError()
            if alt_err < threshold:
                # matched,
                err_index = count
                test_failed = 0
                min_err = alt_err
                img_err = alt_err
                best_img = new_fname
                break
            else:
                if alt_err < min_err:
                    # image is a better match.
                    err_index = count
                    min_err = alt_err
                    img_err = alt_err
                    best_img = new_fname

            count = count + 1
        # closes while loop.

        if test_failed:
            _handleFailedImage(idiff, pngr, best_img)
            # Print for Dart.
            _printDartImageError(img_err, err_index, f_base)
            msg = "Failed image test: %f\n"%idiff.GetThresholdedError()
            raise AssertionError, msg
    # output the image error even if a test passed
    _printDartImageSuccess(img_err, err_index)


def _printDartImageError(img_err, err_index, img_base):
    """Prints the XML data necessary for Dart."""
    print "Failed image test with error: %f"%img_err
    print "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">",
    print "%f </DartMeasurement>"%img_err
    if err_index <= 0:
        print "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>",
    else:
        print "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">",
        print "%d </DartMeasurement>"%err_index
	   
    print "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">",
    print "%s </DartMeasurementFile>"%(img_base + '.test.small.jpg')

    print "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">",
    print "%s </DartMeasurementFile>"%(img_base + '.diff.small.jpg')
    print "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">",
    print "%s </DartMeasurementFile>"%(img_base + '.small.jpg')


def _printDartImageSuccess(img_err, err_index):
    "Prints XML data for Dart when image test succeeded."
    print "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">",
    print "%f </DartMeasurement>"%img_err
    if err_index <= 0:
        print "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>",
    else:
       print "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">",
       print "%d </DartMeasurement>"%err_index
    

def _handleFailedImage(idiff, pngr, img_fname):
    """Writes all the necessary images when an image comparison
    failed."""
    f_base, f_ext = os.path.splitext(img_fname)

    # write out the difference file in full.
    pngw = vtk.vtkPNGWriter()
    pngw.SetFileName(f_base + ".diff.png")
    pngw.SetInput(idiff.GetOutput())
    pngw.Write()
    
    # write the difference image scaled and gamma adjusted for the
    # dashboard.
    sz = pngr.GetOutput().GetDimensions()
    if sz[1] <= 250.0:
        mag = 1.0
    else:
        mag = 250.0/sz[1]

    shrink = vtk.vtkImageResample()
    shrink.SetInput(idiff.GetOutput())
    shrink.InterpolateOn()
    shrink.SetAxisMagnificationFactor(0, mag)
    shrink.SetAxisMagnificationFactor(1, mag)

    gamma = vtk.vtkImageShiftScale()
    gamma.SetInput(shrink.GetOutput())
    gamma.SetShift(0)
    gamma.SetScale(10)

    jpegw = vtk.vtkJPEGWriter()
    jpegw.SetFileName(f_base + ".diff.small.jpg")
    jpegw.SetInput(gamma.GetOutput())
    jpegw.SetQuality(85)
    jpegw.Write()

    # write out the image that was generated.
    shrink.SetInput(idiff.GetInput())
    jpegw.SetInput(shrink.GetOutput())
    jpegw.SetFileName(f_base + ".test.small.jpg")
    jpegw.Write()

    # write out the valid image that matched.
    shrink.SetInput(idiff.GetImage())
    jpegw.SetInput(shrink.GetOutput())
    jpegw.SetFileName(f_base + ".small.jpg")
    jpegw.Write()


def main(cases):
    """ Pass a list of tuples containing test classes and the starting
    string of the functions used for testing.

    Example:

    main ([(vtkTestClass, 'test'), (vtkTestClass1, 'test')])
    """

    processCmdLine()

    timer = vtk.vtkTimerLog()
    s_time = timer.GetCPUTime()
    s_wall_time = time.time()    

    # run the tests
    result = test(cases)

    tot_time = timer.GetCPUTime() - s_time
    tot_wall_time = float(time.time() - s_wall_time)

    # output measurements for Dart
    print "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">",
    print " %f </DartMeasurement>"%tot_wall_time
    print "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">",
    print " %f </DartMeasurement>"%tot_time
    
    if result.wasSuccessful():
        sys.exit(0)
    else:
        sys.exit(1)


def test(cases):
    """ Pass a list of tuples containing test classes and the
    functions used for testing.

    It returns a unittest._TextTestResult object.

    Example:

      test = test_suite([(vtkTestClass, 'test'),
                        (vtkTestClass1, 'test')])
    """
    # Make the test suites from the arguments.
    suites = []
    for case in cases:
        suites.append(unittest.makeSuite(case[0], case[1]))
    test_suite = unittest.TestSuite(suites)

    # Now run the tests.
    runner = unittest.TextTestRunner(verbosity=_VERBOSE)
    result = runner.run(test_suite)

    return result        


def usage():
    msg="""Usage:\nTestScript.py [options]\nWhere options are:\n

    -D /path/to/VTKData
    --data-dir /path/to/VTKData

          Directory containing VTK Data use for tests.  If this option
          is not set via the command line the environment variable
          VTK_DATA_ROOT is used.  If the environment variable is not
          set the value defaults to '../../../../VTKData'.
    
    -B /path/to/valid/image_dir/
    --baseline-root /path/to/valid/image_dir/

          This is a path to the directory containing the valid images
          for comparison.  If this option is not set via the command
          line the environment variable VTK_BASELINE_ROOT is used.  If
          the environment variable is not set the value defaults to
          the same value set for -D (--data-dir).

    -v level
    --verbose level
    
          Sets the verbosity of the test runner.  Valid values are 0,
          1, and 2 in increasing order of verbosity.

    -I
    --interact

          Interacts with the user when chosen.  If this is not chosen
          the test will run and exit as soon as it is finished.  When
          enabled, the behavior of this is rather trivial and works
          best when the test uses Tkinter.

    -n
    --no-image

          Does not do any image comparisons.  This is useful if you
          want to run the test and not worry about test images or
          image failures etc.

    -h
    --help

                 Prints this message.
                 
"""
    return msg
    

def parseCmdLine():
    arguments = sys.argv[1:]

    options = "B:D:v:hnI"
    long_options = ['baseline-root=', 'data-dir=', 'verbose=', 'help',
                    'no-image', 'interact']

    try:
        opts, args = getopt.getopt(arguments, options, long_options)
    except getopt.error, msg:
        print usage()
        print '-'*70
        print msg
        sys.exit (1)
        
    return opts, args

    
def processCmdLine():
    opts, args = parseCmdLine()

    global VTK_DATA_ROOT, VTK_BASELINE_ROOT
    global _VERBOSE, _NO_IMAGE, _INTERACT

    # setup defaults
    try:
        VTK_DATA_ROOT = os.environ['VTK_DATA_ROOT']
    except KeyError:
        VTK_DATA_ROOT = os.path.normpath("../../../../VTKData")

    try:
        VTK_BASELINE_ROOT = os.environ['VTK_BASELINE_ROOT']
    except KeyError:
        pass

    for o, a in opts:
        if o in ('-D', '--data-dir'):
            VTK_DATA_ROOT = os.path.abspath(a)
        if o in ('-B', '--baseline-root'):
            VTK_BASELINE_ROOT = os.path.abspath(a)
        if o in ('-n', '--no-image'):
            _NO_IMAGE = 1
        if o in ('-I', '--interact'):
            _INTERACT = 1
        if o in ('-v', '--verbose'):
            try:
                _VERBOSE = int(a)
            except:
                msg="Verbosity should be an integer.  0, 1, 2 are valid."
                print msg
                sys.exit(1)
        if o in ('-h', '--help'):            
            print usage()
            sys.exit()

    if not VTK_BASELINE_ROOT: # default value.
        VTK_BASELINE_ROOT = VTK_DATA_ROOT



######################################################################
# A Trivial test case to illustrate how this module works.
class SampleTest(vtkTest):
    obj = vtk.vtkActor()
    def testParse(self):
        "Test if class is parseable"
        self._testParse(self.obj)

    def testGetSet(self):
        "Testing Get/Set methods"
        self._testGetSet(self.obj)

    def testBoolean(self):
        "Testing Boolean methods"
        self._testBoolean(self.obj)


if __name__ == "__main__":
    # Test with the above trivial sample test.
    main( [ (SampleTest, 'test') ] )

