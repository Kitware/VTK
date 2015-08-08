#!/usr/bin/env python

import sys
import inspect
import re
import optparse

import vtk
from pydoc import classname

'''
   This is a translation from the TCL code of the same name.
   The original TCL code uses catch {...}
   catch returns 1 if an error occurred an 0 if no error.
   In this code TryUpdate(), TryShutdown(), TrySetInputData and
   TestOne() emulate this by returning False if an error occurred
   and True if no error occurred.
'''
vtk.vtkObject.GlobalWarningDisplayOff()

def RedirectVTKMessages():
    """ Can be used to redirect VTK related error messages to a
    file."""
    log = vtk.vtkFileOutputWindow()
    log.SetFlush(1)
    log.AppendOff()
    log.SetFileName('TestEmptyInput-err.log')
    log.SetInstance(log)


commonExceptions = set([
"vtkDistributedDataFilter",  # core dump
"vtkDataEncoder",  # Use after free error.
"vtkWebApplication",  # Thread issues - calls vtkDataEncoder

 # These give an HDF5 no file name error.
"vtkAMRFlashParticlesReader",
"vtkAMREnzoParticlesReader",
"vtkAMRFlashReader"
])

classLinuxExceptions = set([
"vtkAMREnzoReader"  # core dump
])

# In the case of Windows, these classes cause a crash.
classWindowsExceptions = set([
"vtkWin32VideoSource",  # Update() works the first time but a subsequent run calls up the webcam which crashes on close.

"vtkCMLMoleculeReader",
"vtkCPExodusIIInSituReader",
"vtkMINCImageWriter",
"vtkQtInitialization"
])

classExceptions = set()

emptyPD = vtk.vtkPolyData()
emptyID = vtk.vtkImageData()
emptySG = vtk.vtkStructuredGrid()
emptyUG = vtk.vtkUnstructuredGrid()
emptyRG = vtk.vtkRectilinearGrid()

# This will hold the classes to be tested.
vtkClasses = set()
classNames = None
classesTested = set()

# Keep a record of the classes tested.
nonexistentClasses = set()
abstractClasses = set()
noConcreteImplementation = set()
noShutdown = set()
noObserver = set()
# Is a vtkAlgorithm but EmptyInput failed.
isVtkAlgorithm = set()
emptyInputWorked = set()
emptyInputFailed = set()
#------------------------
# These variables are used for further record keeping
# should users wish to investigate or debug further.
noUpdate = set()
noSetInput = set()

# A dictionary consisting of a key and five booleans corresponding to
# whether SetInput() using emptyPD, emptyID, emptySG, emptyUG, emptyRG
# worked.
inputStatus = dict()

# A dictionary consisting of a key and five booleans corresponding to
# whether Update() using emptyPD, emptyID, emptySG, emptyUG, emptyRG
# worked if SetInput() worked.
updateStatus = dict()
#-----------------------

# Controls the verbosity of the output.
verbose = False


class ErrorObserver:
   '''
   See: http://public.kitware.com/pipermail/vtkusers/2012-June/074703.html
   '''
   def __init__(self):
       self.__ErrorOccurred = False
       self.__ErrorMessage = None
       self.CallDataType = 'string0'

   def __call__(self, obj, event, message):
       self.__ErrorOccurred = True
       self.__ErrorMessage = message

   def ErrorOccurred(self):
       occ = self.__ErrorOccurred
       self.__ErrorOccurred = False
       return occ

   def ErrorMessage(self):
       return self.__ErrorMessage

def GetVTKClasses():
    '''
    :return: a set of all the VTK classes.
    '''
    # This pattern will get the name and type of the member in the vtk classes.
    pattern = r'\<vtkclass (.*)\.(.*)\>'
    regEx = re.compile(pattern)
    vtkClasses = inspect.getmembers(
                    vtk, inspect.isclass and not inspect.isabstract)
    res = set()
    for name, obj in vtkClasses:
            result = re.match(regEx, repr(obj))
            if result:
                res.add(result.group(2))
    return res

def GetVTKClassGroups(vtkClasses, subStr):
    '''
    :param: vtkClasses - the set of VTK classes
    :param: subStr - the substring for the VTK class to match e.g Readers
    :return: a set of all the VTK classes that are readers.
    '''
    res = set()
    for obj in list(vtkClasses):
            if obj.find(subStr) > -1:
                res.add(obj)
    return res

def FilterClasses(allClasses, filter):
    '''
    :param: vtkCLasses - the set of VTK classes
    :param: filters - a list of substrings of classes to be removed
    :return: a set of all the VTK classes that do not have the substrings
     in their names.
    '''
    res = allClasses
    for f in filter:
        c = GetVTKClassGroups(allClasses, f)
        res = res - c
    return res

def TryUpdate(b, e):
    '''
    The method Update() is tried on the instantiated class.

    Some classes do not have an Update method, if this is the case,
    Python will generate an AttributeError, in this case, the name
    of the class is stored in the global variable noUpdate and false
    is returned.

    If an error occurs on Update() then the error handler
    will be triggered and in this case false is returned.

    :param: b - the class on which Update() will be tried.
    :param: e - the error handler.
    :return: True if the update was successful, False otherwise.
    '''
    try:
        b.Update()
        if e.ErrorOccurred():
            return False
        else:
            return True
    except AttributeError:
        # No Update() method
        noUpdate.add(b.GetClassName())
        return False
    except:
        raise

def TryShutdown(b, e):
    '''
    The method Shutdown() is tried on the instantiated class.

    Some classes do not have an Shutdown method, if this is the case,
    Python will generate an AttributeError, in this case, the name
    of the class is stored in the global variable noUpdate and False
    is returned.

    If an error occurs on Shutdown() then the error handler
    will be triggered and in this case False is returned.

    :param: b - the class on which Shutdown() will be tried.
    :param: e - the error handler.
    :return: True if the update was successful, False otherwise.
    '''
    try:
        b.Shutdown()
        if e.ErrorOccurred():
            return False
        else:
            return True
    except AttributeError:
        # No Shutdown() method
        noShutdown.add(b.GetClassName())
        return False
    except:
        raise

def TrySetInputData(b, e, d):
    '''
    The method SetInputData() is tried on the instantiated class.

    Some classes do not have an SetInputData method, if this is the case,
    Python will generate an AttributeError, in this case, the name
    of the class is stored in the global variable noUpdate and False
    is returned.

    If an error occurs on SetInputData() then the error handler
    will be triggered and in this case False is returned.

    :param: b - the class on which SetInputData() will be tried.
    :param: e - the error handler.
    :param: d - input data.
    :return: True if the update was successful, False otherwise.
    '''
    try:
        b.SetInputData(d)
        if e.ErrorOccurred():
            return False
        else:
            return True
    except AttributeError:
        # No SetInputData() method
        noSetInput.add(b.GetClassName())
        return False
    except:
        raise

def TestOne(cname):
    '''
    Test the named class.

    Some classes will generate a TypeError or an AttributeError,
    in this case, the name of the class is stored in the global variable
    abstractClasses or noConcreteImplementation with False being returned.

    Return values:
    0: If not any of the SetInput() tests and the corresponding
             Update() are both true.
    1: If at least one of SetInput() tests and the corresponding
             Update() are both true.
    2: No observer could be added.
    3: If it is an abstract class.
    4: No concrete implementation.
    5: Class does not exist.

    :param: cname - the name of the class to be tested.
    :return: One of the above return values.
    '''
    try:
        b = getattr(vtk, cname)()
        e = ErrorObserver()
        isAlgorithm = False
        # A record of whether b.SetInput() worked or not.
        # The indexing corresponds to using
        # emptyPD, emptyID, emptySG, emptyUG, emptyRG in SetInput()
        iStatus = [False, False, False, False, False]
        # A record of whether b.Update() worked or not.
        # The indexing corresponds to:
        # [emptyPD, emptyID, emptySG, emptyUG, emptyRG]
        uStatus = [False, False, False, False, False]
        try:
            b.AddObserver('ErrorEvent', e)
        except AttributeError:
            noObserver.add(cname)
            return 2
        except:
            raise
        if b.IsA("vtkAlgorithm"):
            u = TryUpdate(b, e)
            if u:
                isAlgorithm = True

        if TrySetInputData(b, e, emptyPD):
            iStatus[0] = True
            u = TryUpdate(b, e)
            if u:
                uStatus[0] = True

        if TrySetInputData(b, e, emptyID):
            iStatus[1] = True
            u = TryUpdate(b, e)
            if u:
                uStatus[1] = True

        if TrySetInputData(b, e, emptySG):
            iStatus[2] = True
            u = TryUpdate(b, e)
            if u:
                uStatus[2] = True

        if TrySetInputData(b, e, emptyUG):
            iStatus[3] = True
            u = TryUpdate(b, e)
            if u:
                uStatus[3] = True

        if TrySetInputData(b, e, emptyRG):
            iStatus[4] = True
            u = TryUpdate(b, e)
            if u:
                uStatus[4] = True

        # If thread creation moves away from the vtkGeoSource constructor, then
        # this ShutDown call will not be necessary...
        #
        if b.IsA("vtkGeoSource"):
            TryShutdown(b, e)

        inputStatus[cname] = iStatus
        updateStatus[cname] = uStatus
        ok = False
        # We require input and update to work for success.
        mergeStatus = map(lambda pair: pair[0] & pair[1], zip(iStatus, uStatus))
        for value in mergeStatus:
            ok |= value
        if not(ok) and isAlgorithm:
                isVtkAlgorithm.add(cname)
        if ok:
            emptyInputWorked.add(cname)
        else:
            emptyInputFailed.add(cname)
        b = None
        if ok:
             return 1
        return 0
    except TypeError:
        # Trapping abstract classes.
        abstractClasses.add(cname)
        return 3
    except NotImplementedError:
        # No concrete implementation
        noConcreteImplementation.add(cname)
        return 4
    except AttributeError:
        # Class does not exist
        nonexistentClasses.add(cname)
        return 5
    except:
        raise

def TestEmptyInput(batch, batchNo=0, batchSize=0):
    '''
    Test each class in batch for empty input.

    :param: batch - the set of classes to be tested.
    :param: batchNo - if the set of classes is a subgroup then this
                     is the index of the subgroup.
    :param: batchSize - if the set of classes is a subgroup then this
                     is the size of the subgroup.
    '''
    baseIdx = batchNo * batchSize
    idx = baseIdx
    for a in batch:
        batchIdx = idx - baseIdx
        # res = " Testing -- {:4d} - {:s}".format(idx,a)
        # There is no format method in Python 2.5
        res = " Testing -- %4d - %s" % (idx, a)
        if (batchIdx < len(batch) - 1):
            # nextRes = " Next -- {:4d} - {:s}".format(idx + 1,list(batch)[batchIdx +1])
            nextRes = " Next -- %4d - %s" % (idx + 1, list(batch)[batchIdx + 1])
        else:
            nextRes = "No next"
#         if verbose:
#             print(res, nextRes)
        classesTested.add(a)
        ok = TestOne(a)
        if ok == 0:
            if verbose:
                print(res + ' - Fail')
        elif ok == 1:
            if verbose:
                print(res + ' - Ok')
        elif ok == 2:
            if verbose:
                print(res + ' - no observer could be added.')
        elif ok == 3:
            if verbose:
                print(res + ' - is Abstract')
        elif ok == 4:
            if verbose:
                print(res + ' - No concrete implementation')
        elif ok == 5:
            if verbose:
                print(res + ' - Does not exist')
        else:
            if verbose:
                print(res + ' - Unknown status')
        idx += 1

def BatchTest(vtkClasses, batchNo, batchSize):
    '''
    Batch the classes into groups of batchSize.

    :param: batchNo - if the set of classes is a subgroup then this
                     is the index of the subgroup.
    :param: batchSize - if the set of classes is a subgroup then this
                     is the size of the subgroup.
    '''
    idx = 0
    total = 0;
    batch = set()
    for a in vtkClasses:
        currentBatchNo = idx // batchSize;
        if currentBatchNo == batchNo:
            batch.add(a)
            total += 1
            if total == batchSize:
                TestEmptyInput(batch, batchNo, batchSize)
                print(total)
                batch = set()
                total = 0
        idx += 1
    if batch:
        TestEmptyInput(batch, batchNo, batchSize)
        print(total)

def PrintResultSummary():
    print('-' * 40)
    print('Empty Input worked: %i' % len(emptyInputWorked))
    print('Empty Input failed: %i' % len(emptyInputFailed))
    print('Abstract classes: %i' % len(abstractClasses))
    print('Non-existent classes: %i' % len(nonexistentClasses))
    print('No concrete implementation: %i' % len(noConcreteImplementation))
    print('No observer could be added: %i' % len(noObserver))
    print('-' * 40)
    print('Total number of classes tested: ', len(classesTested)) # , classesTested
    print('-' * 40)
    print('Excluded from testing: ', len(classExceptions))
    print('-' * 40)

def ProgramOptions():
    desc = """
    %prog Tests each VTK class for empty input using various data structures.
    """
    parser = optparse.OptionParser(description=desc)
    parser.set_defaults(verbose=False)

    parser.add_option('-c', '--classnames',
        help='The name of the class or a list of classes in quotes separated by commas.',
        type='string',
        dest='classnames',
        default=None,
        action='store')
    parser.add_option('-q', '--quiet',
        help='Do not print status messages to stdout (default)',
        dest='verbose',
        action="store_false")
    parser.add_option('-v', '--verbose',
        help='Print status messages to stdout',
        dest='verbose',
        action="store_true")

    (opts, args) = parser.parse_args()

    return (True, opts)

def CheckPythonVersion(ver):
    '''
    Check the Python version.

    :param: ver - the minimum required version number as hexadecimal.
    :return: True if if the Python version is greater than or equal to ver.
    '''
    if sys.hexversion < ver:
        return False
    return True

def main(argv=None):
    if not CheckPythonVersion(0x02060000):
        print('This program requires Python 2.6 or greater.')
        return

    global classExceptions
    global vtkClasses
    global classNames
    global verbose

    if argv is None:
        argv = sys.argv

    (res, opts) = ProgramOptions()
    if opts.classnames:
        cn = [x.strip() for x in opts.classnames.split(',')]
        classNames = set(cn)
    if opts.verbose:
        verbose = opts.verbose

    print('CTEST_FULL_OUTPUT (Avoid ctest truncation of output)')

    # RedirectVTKMessages()
    if classNames:
        TestEmptyInput(classNames)
    else:
        classExceptions = commonExceptions.union(classLinuxExceptions)
        classExceptions = classExceptions.union(classWindowsExceptions)

        vtkClasses = GetVTKClasses()
#         filter = ['Reader', 'Writer', 'Array_I', 'Qt']
#         vtkClasses = FilterClasses(vtkClasses, filter)
        vtkClasses = vtkClasses - classExceptions
        TestEmptyInput(vtkClasses)
#         In Windows
#         0-10, 10-17, 17-18, 18-23 in steps of 100 work but not if called
#         in a loop.
#         intervals = [[18,20]]# [[0,10]], [10,17], [17,18], [18,20]]
#         for j in intervals:
#             for i in range(j[0],j[1]):
#                 BatchTest(vtkClasses, i, 100)

    PrintResultSummary()

if __name__ == '__main__':
    sys.exit(main())
