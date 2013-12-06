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

import vtk
import server

# Try standard Python imports
try :
    import os, re, time, datetime, threading, imp, inspect, Queue, types, io
except :
    import_warning_info += "\nUnable to load at least one basic Python module"

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
    from selenium import webdriver
except :
    import_warning_info += "\nUnable to load at least one module necessary for browser tests"

# HTTP imports
try :
    import requests
except :
    import_warning_info += "\nUnable to load at least one module necessary for HTTP tests"


# Define some infrastructure to support different (or no) browsers
test_module_browsers = ["firefox", "chrome", "internet_explorer", "safari", "nobrowser"]
TestModuleBrowsers = type("Enum", (), {k: i for i, k in enumerate(test_module_browsers)})


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

      --test-use-browser => This should be one of the supported browser types,
      or else 'nobrowser'.  The choices are 'chrome', 'firefox', 'internet_explorer',
      'safari', or 'nobrowser'.
    """

    parser.add_argument("--run-test-script",
                        default="",
                        help="The path to a test script to run",
                        dest="testScriptPath")

    parser.add_argument("--baseline-img-dir",
                        default="",
                        help="The path to the directory containing the web test baseline images",
                        dest="baselineImgDir")

    parser.add_argument("--test-use-browser",
                        default="nobrowser",
                        help="One of 'chrome', 'firefox', 'internet_explorer', 'safari', or 'nobrowser'.",
                        dest="useBrowser")

    parser.add_argument("--temporary-directory",
                        default=".",
                        help="A temporary directory for storing test images and diffs",
                        dest="tmpDirectory")

    parser.add_argument("--test-image-file-name",
                        default="",
                        help="Name of file in which to store generated test image",
                        dest="testImgFile")


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

    t = threading.Thread(target=launch_web_test,
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
# So we can change our time format in a single place, this function is
# provided.
# =============================================================================
def get_current_time_string() :
    """
    This function returns the current time as a string, using ISO 8601 format.
    """

    return datetime.datetime.now().isoformat(" ")


# =============================================================================
# Uses vtkTesting to compare images.  According to comments in the vtkTesting
# C++ code (and this seems to work), if there are multiple baseline images in
# the same directory as the baseline_img, and they follow the naming pattern:
# 'img.png', 'img_1.png', ... , 'img_N.png', then all of these images will be
# tried for a match.
# =============================================================================
def compare_images(test_img, baseline_img, tmp_dir="."):
    """
    This function creates a vtkTesting object, and specifies the name of the
    baseline image file, using a fully qualified path (baseline_img must be
    fully qualified).  Then it calls the vtkTesting method which compares the
    image (test_img, specified only with a relative path) against the baseline
    image as well as any other images in the same directory as the baseline
    image which follow the naming pattern: 'img.png', 'img_1.png', ... , 'img_N.png'

        test_img: File name of output image to be compared agains baseline.

        baseline_img: Fully qualified path to first of the baseline images.

        tmp_dir: Fully qualified path to a temporary directory for storing images.
    """

    # Create a vtkTesting object and specify a baseline image
    t = vtk.vtkTesting()
    t.AddArgument("-T")
    t.AddArgument(tmp_dir)
    t.AddArgument("-V")
    t.AddArgument(baseline_img)

    # Perform the image comparison test and print out the result.
    return t.RegressionTest(test_img, 0.0)


# =============================================================================
# Provide a wait function
# =============================================================================
def wait_with_timeout(delay=None, limit=0, criterion=None):
    """
    This function provides the ability to wait for a certain number of seconds,
    or else to wait for a specific criterion to be met.
    """
    for i in itertools.count():
        if criterion is not None and criterion():
            return True
        elif delay * i > limit:
            return False
        else:
            time.sleep(delay)


# =============================================================================
# Define a WebTest class with five stages of testing: initialization, setup,
# capture, postprocess, and cleanup.
# =============================================================================
class WebTest(object) :
    """
    This is the base class for all automated web-based tests.  It defines five
    stages that any test must run through, and allows any or all of these
    stages to be overridden by subclasses.  This class defines the run_test
    method to invoke the five stages overridden by subclasses, one at a time:
    1) initialize, 2) setup, 3) capture, 4) postprocess, and 5) cleanup.
    """
    class Abort:
        pass

    def __init__(self, url=None, testname=None, **kwargs) :
        self.url = url
        self.testname = testname

    def run_test(self):
        try:
            self.initialize()
            self.setup()
            self.capture()
            self.postprocess()
        except WebTest.Abort:
            # Placeholder for future option to return failure result
            pass
        except :
            self.cleanup()
            raise

        self.cleanup()

    def initialize(self):
        pass

    def setup(self):
        pass

    def capture(self):
        pass

    def postprocess(self):
        pass

    def cleanup(self):
        pass


# =============================================================================
# Define a WebTest subclass designed specifically for browser-based tests.
# =============================================================================
class BrowserBasedWebTest(WebTest):
    """
    This class can be used as a base for any browser-based web tests.  It
    introduces the notion of a selenium browser and overrides phases (1) and
    (3), initialization and cleanup, of the test phases introduced in the base
    class.  Initialization involves selecting the browser type, setting the
    browser window size, and asking the browser to load the url.  Cleanup
    involves closing the browser window.
    """
    def __init__(self, size=None, browser=None, **kwargs):
        self.size = size
        self.browser = browser
        self.window = None

        WebTest.__init__(self, **kwargs)

    def initialize(self):
        if self.browser is None or self.browser == TestModuleBrowsers.chrome:
            self.window = webdriver.Chrome()
        elif self.browser == TestModuleBrowsers.firefox:
            self.window = webdriver.Firefox()
        elif self.browser == TestModuleBrowsers.internet_explorer:
            self.window = webdriver.Ie()
        else:
            raise ValueError("self.browser argument has illegal value %r" % (self.browser))

        if self.size is not None:
            self.window.set_window_size(self.size[0], self.size[1])

        if self.url is not None:
            self.window.get(self.url)

    def cleanup(self):
        self.window.quit()


# =============================================================================
# Extend BrowserBasedWebTest to handle vtk-style image comparison
# =============================================================================
class ImageComparatorWebTest(BrowserBasedWebTest):
    """
    This class extends browser based web tests to include image comparison.  It
    overrides the capture phase of testing with some functionality to simply
    grab a screenshot of the entire browser window.  It overrides the
    postprocess phase with a call to vtk image comparison functionality.
    Derived classes can then simply override the setup function with a series
    of selenium-based browser interactions to create a complete test.  Derived
    classes may also prefer to override the capture phase to capture only
    certain portions of the browser window for image comparison.
    """
    def __init__(self, filename=None, baseline=None, temporaryDir=None, **kwargs):
        if filename is None:
            raise TypeError("missing argument 'filename'")
        if baseline is None:
            raise TypeError("missing argument 'baseline'")

        BrowserBasedWebTest.__init__(self, **kwargs)
        self.filename = filename
        self.baseline = baseline
        self.tmpDir = temporaryDir

    def capture(self):
        self.window.save_screenshot(self.filename)

    def postprocess(self):
        result = compare_images(self.filename, self.baseline, self.tmpDir)

        if result == 1 :
            test_pass(self.testname)
        else :
            test_fail(self.testname)


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
# Combines a variation on above function with the write_image_to_disk function.
# converting jpg to png in the process, if necessary.
# =============================================================================
def save_image_data_as_png(browser, cssSelector, imgfilename) :
    """
    This function takes a selenium browser instance, a css selector string,
    and a file name.  It uses the css selector string to finds the target HTML
    Image element, which should contain a Base64 encoded JPEG image string,
    it decodes the string to image data, and then saves the data to the file.
    The image type of the written file is determined from the extension of the
    provided filename.

        browser: A selenium browser instance as created by webdriver.Chrome(),
        for example.

        cssSelector: A string containing a CSS selector which will be used to
        find the HTML image element of interest.

        imgFilename: The filename to which to save the image. The extension is
        used to determine the type of image which should be saved.
    """
    imageElt = browser.find_element_by_css_selector(cssSelector)
    base64String = imageElt.get_attribute("src")
    b64RegEx = re.compile(ur'data:image/jpeg;base64,(.+)', re.UNICODE)
    b64Matcher = b64RegEx.match(base64String)
    img = Image.open(io.BytesIO(base64.b64decode(b64Matcher.group(1))))
    img.save(imgfilename)


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
# There could be problems if the script file has more than one class defn which
# is a subclass of vtk.web.testing.WebTest, so we should write some
# documentation to help people avoid that.
# =============================================================================
def instantiate_test_subclass(pathToScript, **kwargs) :
    """
    This function takes the fully qualified path to a test file, along with
    any needed keyword arguments, then dynamically loads the file as a module
    and finds the test class defined inside of it via inspection.  It then
    uses the keywork arguments to instantiate the test class and return the
    instance.

        pathToScript: Fully qualified path to python file containing defined
        subclass of one of the test base classes.
        kwargs: Keyword arguments to be passed to the constructor of the
        testing subclass.
    """

    # Load the file as a module
    moduleName = imp.load_source('dynamicTestModule', pathToScript)
    instance = None

    # Inspect dynamically loaded module members
    for name, obj in inspect.getmembers(moduleName) :
        # Looking for classes only
        if inspect.isclass(obj) :
            instance = obj.__new__(obj)
            # And only classes defined in the dynamically loaded module
            if instance.__module__ == 'dynamicTestModule' :
                try :
                    instance.__init__(**kwargs)
                    break;
                except Exception as inst:
                    print 'Caught exception: ' + str(type(inst))
                    print inst
                    raise

    return instance


# =============================================================================
# For testing purposes, define a function which can interact with a running
# paraview or vtk web application service.
# =============================================================================
def launch_web_test(*args, **kwargs) :
    """
    This function loads a python file as a module (with no package), and then
    instantiates the class it must contain, and finally executes the run_test()
    method of the class (which the class may override, but which is defined in
    both of the testing base classes, WebTest and ImageComparatorBaseClass).
    After the run_test() method finishes, this function will stop the web
    server if required.  This function expects some keyword arguments will be
    present in order for it to complete it's task:

        kwargs['serverHandle']: A reference to the vtk.web.server should be
        passed in if this function is to stop the web service after the test
        is finished.  This should normally be the case.

        kwargs['serverOpts']: An object containing all the parameters used
        to start the web service.  Some of them will be used in the test script
        in order perform the test.  For example, the port on which the server
        was started will be required in order to connect to the server.

        kwargs['testScript']: The full path to the python file containing the
        testing subclass.
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
        # print 'These are the serverOpts we got: '
        # print serverOpts

    # Get the full path to the test script
    if 'testScript' in kwargs :
        testScriptFile = kwargs['testScript']

    testName = 'unknown'

    # Check for a test file (python file)
    if testScriptFile is None :
        print 'No test script file found, no test script will be run.'
        test_fail(testName)

    # The test name will be generated from the python script name, so
    # match and capture a bunch of contiguous characters which are
    # not '.', '\', or '/', followed immediately by the string '.py'.
    fnamePattern = re.compile('([^\.\/\\\]+)\.py')
    fmatch = re.search(fnamePattern, testScriptFile)
    if fmatch :
        testName = fmatch.group(1)
    else :
        print 'Unable to parse testScriptFile (' + str(testScriptfile) + '), no test will be run'
        test_fail(testName)

    # If we successfully got a test name, we are ready to try and run the test
    if testName != 'unknown' :

        # Output file and baseline file names are generated from the test name
        imgFileName = testName + '.png'
        knownGoodFileName = concat_paths(serverOpts.baselineImgDir, imgFileName)
        tempDir = serverOpts.tmpDirectory
        testImgFileName = serverOpts.testImgFile

        testBrowser = test_module_browsers.index(serverOpts.useBrowser)

        # Now try to instantiate and run the test
        try :
            testInstance = instantiate_test_subclass(testScriptFile,
                                                     testname=testName,
                                                     host=serverOpts.host,
                                                     port=serverOpts.port,
                                                     browser=testBrowser,
                                                     filename=testImgFileName,
                                                     baseline=knownGoodFileName,
                                                     temporaryDir=tempDir)

            # If we were able to instantiate the test, run it, otherwise we
            # consider it a failure.
            if testInstance is not None :
                testInstance.run_test()
            else :
                print 'Unable to instantiate test instance, failing test'
                test_fail(testName)
                return

        except Exception as inst :
            print 'Caught an exception while running test script:'
            print '  ' + str(type(inst))
            print '  ' + str(inst)
            test_fail(testName)

    # If we were passed a server handle, then use it to stop the service
    if serverHandle is not None :
        serverHandle.stop_webserver()


# =============================================================================
# To keep the service module clean, we'll process the test results here, given
# the test result object we generated in "launch_web_test".  It is
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
