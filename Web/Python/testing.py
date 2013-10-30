r"""
    This module provides some testing functionality for paraview and
    vtk web applications.  It provides the ability to run an arbitrary
    test script in a separate thread and communicate the results back
    to the service so that the CTest framework can be notified of the
    success or failure of the test.

    This test harness will notice when the test script has finished
    running and will notify the service to stop.  At this point, the
    test results will be checked in the main thread which ran the
    service, and in the case of failure an exeception will be raised
    to notify CTest of the failure.

    Test scripts need to follow some simple rules in order to work
    within the test harness framework:

    1) implement a function called "runTest(args)", where the args
    parameter contains all the arguments given to the web application
    upon starting.  Among other important items, args will contain the
    port number where the web application is listening.

    2) import the testing module so that the script has access to
    the functions which indicate success and failure.  Also the
    testing module contains convenience functions that might be of
    use to the test scripts.

       from vtk.web import testing

    3) Call the "testPass(testName)" or "testFail(testName)" functions
    from within the runTest(args) function to indicate to the framework
    whether the test passed or failed.

"""

import_warning_info = ""
test_module_comm_queue = None

import server

import os, re, time, datetime, threading, imp, Queue

# Image comparison imports
try:
    import Image
    import base64
    import itertools
except:
    import_warning_info += "\nUnable to load at least one modules necessary for image comparison"

# Browser testing imports
try :
    import selenium
except :
    import_warning_info += "\nUnable to load at least one module necessary for browser tests"

# HTTP imports
try :
    import requests
except :
    import_warning_info += "\nUnable to load at least one module necessary for HTTP tests"


# =============================================================================
# Checks whether test script supplied, if so, safely imports needed modules
# =============================================================================
def initialize(opts, reactor=None) :
    """
    This function should be called to initialize the testing module.  The first
    important thing it does is to store the options for later, since the
    startTestThread function will need them.  Then it checks the arguments that
    were passed into the server to see if a test was actually requested, making
    a note of this fact.  Then, if a test was required, this function then
    checks if all the necessary testing modules were safely imported, printing
    a warning if not.  If tests were requested and all modules were present,
    then this function sets "test_module_do_testing" to True and sets up the
    startTestThread function to be called after the reactor is running.
    """

    global import_warning_info

    global testModuleOptions
    testModuleOptions = opts

    # Check if a test was actually requested
    if (testModuleOptions.testScriptPath != "" and testModuleOptions.testScriptPath is not None) :
        # Check if we ran into trouble with any of the testing imports
        if import_warning_info != "" :
            print "WARNING: Testing will cannot be enabled for the following reasons:"
            print import_warning_info
        else :
            if reactor is not None :
                # Add startTest callback to the reactor callback queue, so that
                # the test thread get started after the reactor is running.  Of
                # course this should only happen if everything is good for tests.
                reactor.callWhenRunning(_start_test_thread)
            else :
                # Otherwise, our aim is to start the thread from another process
                # so just call the start method.
                _start_test_thread()


# =============================================================================
# Grab out the command-line arguments needed for by the testing module.
# =============================================================================
def add_arguments(parser) :
    """
    This function retrieves any command-line arguments that the client-side
    tester needs.  In order to run a test, you will typically just need the
    following:

      --run-test-script => This should be the full path to the test script to
      be run.

      --baseline-img-dir => This should be the 'Baseline' directory where the
      baseline images for this test are located.
    """

    parser.add_argument("--run-test-script",
                        default="",
                        help="The path to a test script to run",
                        dest="testScriptPath")

    parser.add_argument("--baseline-img-dir",
                        default="",
                        help="The path to the directory containing the web test baseline images",
                        dest="baselineImgDir")


# =============================================================================
# Initialize the test client
# =============================================================================
def _start_test_thread() :
    """
    This function checks whether testing is required and if so, sets up a Queue
    for the purpose of communicating with the thread.  then it starts the
    after waiting 5 seconds for the server to have a chance to start up.
    """

    global test_module_comm_queue
    test_module_comm_queue = Queue.Queue()

    t = threading.Thread(target=interact_with_web_visualizer,
                         args = [],
                         kwargs = { 'serverOpts': testModuleOptions,
                                    'commQueue': test_module_comm_queue,
                                    'serverHandle': server,
                                    'testScript': testModuleOptions.testScriptPath })

    t.start()


# =============================================================================
# Test scripts call this function to indicate passage of their test
# =============================================================================
def test_pass(testName) :
    """
    Test scripts should call this function to indicate that the test passed.  A
    note is recorded that the test succeeded, and is checked later on from the
    main thread so that CTest can be notified of this result.
    """

    global test_module_comm_queue
    resultObj = { testName: 'pass' }
    test_module_comm_queue.put(resultObj)


# =============================================================================
# Test scripts call this function to indicate failure of their test
# =============================================================================
def test_fail(testName) :
    """
    Test scripts should call this function to indicate that the test failed.  A
    note is recorded that the test did not succeed, and this note is checked
    later from the main thread so that CTest can be notified of the result.

    The main thread is the only one that can signal test failure in
    CTest framework, and the main thread won't have a chance to check for
    passage or failure of the test until the main loop has terminated.  So
    here we just record the failure result, then we check this result in the
    processTestResults() function, throwing an exception at that point to
    indicate to CTest that the test failed.
    """

    global test_module_comm_queue
    resultObj = { testName: 'fail' }
    test_module_comm_queue.put(resultObj)


# =============================================================================
# Concatenate any number of strings into a single path string.
# =============================================================================
def concat_paths(*pathElts) :
    """
    A very simple convenience function so that test scripts can build platform
    independent paths out of a list of elements, without having to import the
    os module.

        pathElts: Any number of strings which should be concatenated together
        in a platform independent manner.
    """

    return os.path.join(*pathElts)


# =============================================================================
# I took this function out of Roni's vtkwebtest.py python code.  Eventually I
# believe there will be a lightweight vtk library for doing image comparisons
# which will be wrapped from python, and then we'll use that.
# =============================================================================
def compare_images(file1, file2):
    """
    Compare two images, pixel by pixel, summing up the differences in every
    component and every pixel.  Return the magnitude of the difference between
    the two images.

        file1: A path to the first image file on disk
        file2: A path to the second image file on disk
    """

    img1 = Image.open(file1)
    img2 = Image.open(file2)

    if img1.size[0] != img2.size[0] or img1.size[1] != img2.size[1]:
        raise ValueError("Images are of different sizes: img1 = (" +
                         str(img1.size[0]) + " x " + str(img1.size[1]) +
                         ") , img2 = (" + str(img2.size[0]) + " x " +
                         str(img2.size[1]) + ")")

    size = img1.size

    img1 = img1.load()
    img2 = img2.load()

    indices = itertools.product(range(size[0]), range(size[1]))
    diff = 0

    for i, j in indices:
        p1 = img1[i, j]
        p2 = img2[i, j]
        diff += abs(p1[0] - p2[0]) + abs(p1[1] - p2[1]) + abs(p1[2] - p2[2])

    return diff


# =============================================================================
# Given a css selector to use in finding the image element, get the element,
# then base64 decode the "src" attribute and return it.
# =============================================================================
def get_image_data(browser, cssSelector) :
    """
    This function takes a selenium browser and a css selector string and uses
    them to find the target HTML image element.  The desired image element
    should contain it's image data as a Base64 encoded JPEG image string.
    The 'src' attribute of the image is read, Base64-decoded, and then
    returned.

        browser: A selenium browser instance, as created by webdriver.Chrome(),
        for example.

        cssSelector: A string containing a CSS selector which will be used to
        find the HTML image element of interest.
    """

    # Here's maybe a better way to get at that image element
    imageElt = browser.find_element_by_css_selector(cssSelector)

    # Now get the Base64 image string and decode it into image data
    base64String = imageElt.get_attribute("src")
    b64RegEx = re.compile(ur'data:image/jpeg;base64,(.+)', re.UNICODE)
    b64Matcher = b64RegEx.match(base64String)
    imgdata = base64.b64decode(b64Matcher.group(1))

    return imgdata


# =============================================================================
# Given a decoded image and the full path to a file, write the image to the
# file.
# =============================================================================
def write_image_to_disk(imgData, filePath) :
    """
    This function takes an image data, as returned by this module's
    get_image_data() function for example, and writes it out to the file given by
    the filePath parameter.

        imgData: An image data object
        filePath: The full path, including the file name and extension, where
        the image should be written.
    """

    with open(filePath, 'wb') as f:
        f.write(imgData)


# =============================================================================
# For testing purposes, define a function which can interact with a running
# paraview or vtk web application service.  This function can do multiple
# tests with a running server before signalling the server to shut down.
# In practice, however, we may want to stop and restart the service for each
# test we do, in order to avoid
# =============================================================================
def interact_with_web_visualizer(*args, **kwargs) :
    """
    This function loads a test script as a module (with no package), and then
    executes the runTest() function which the script must contain.  After the
    test script finishes, this function will stop the web server if required.
    This function expects some keyword arguments will be present in order for
    it to complete it's task:

        kwargs['serverHandle']: A reference to the vtk.web.server should be
        passed in if this function is to stop the web service after the test
        is finished.  This should normally be the case.

        kwargs['serverOpts']: An object containing all the parameters used
        to start the web service.  Some of them will be used in the test script
        in order perform the test.  For example, the port on which the server
        was started will be required in order to connect to the server.

        kwargs['testScript']: The full path to the python script which this
        function will run.
    """

    serverHandle = None
    serverOpts = None
    testScriptFile = None

    # If we got one of these, we'll use it to stop the server afterward
    if 'serverHandle' in kwargs :
        serverHandle = kwargs['serverHandle']

    # This is really the thing all test scripts will need: access to all
    # the options used to start the server process.
    if 'serverOpts' in kwargs :
        serverOpts = kwargs['serverOpts']

    # Get the full path to the test script
    if 'testScript' in kwargs :
        testScriptFile = kwargs['testScript']

    # Now load the test script as a module, given the full path
    if testScriptFile is None :
        print 'No test script file found, no test script will be run.'
        return

    moduleName = imp.load_source('', testScriptFile)

    # Now try to run the script's "runTest()" function
    try :
        moduleName.runTest(serverOpts)
    except Exception as inst :
        print 'Caught an exception while running test script:'
        print '  ' + str(type(inst))
        print '  ' + str(inst)
        test_fail(testScriptFile)

    # If we were passed a server handle, then use it to stop the service
    if serverHandle is not None :
        serverHandle.stop_webserver()


# =============================================================================
# So we can change our time format in a single place, this function is
# provided.
# =============================================================================
def get_current_time_string() :
    """
    This function returns the current time as a string, using ISO 8601 format.
    """

    return datetime.datetime.now().isoformat(" ")


# =============================================================================
# To keep the service module clean, we'll process the test results here, given
# the test result object we generated in "interact_with_veb_visualizer".  It is
# passed back to this function after the service has completed.  Failure of
# of the test is indicated by raising an exception in here.
# =============================================================================
def finalize() :
    """
    This function checks the module's global test_module_comm_queue variable for a
    test result.  If one is found and the result is 'fail', then this function
    raises an exception to communicate the failure to the CTest framework.

    In order for a test result to be found in the test_module_comm_queue variable,
    the test script must have called either the testPass or testFail functions
    provided by this test module before returning.
    """

    global test_module_comm_queue

    if test_module_comm_queue is not None :
        resultObject = test_module_comm_queue.get()

        failedATest = False

        for testName in resultObject :
            testResult = resultObject[testName]
            if testResult == 'fail' :
                print '  Test -> ' + testName + ': ' + testResult
                failedATest = True

        if failedATest is True :
            raise Exception("At least one of the requested tests failed.  " +
                            "See detailed output, above, for more information")
