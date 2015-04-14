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
"vtkDistributedDataFilter", # core dump
"vtkDataEncoder", # Use after free error.

 # These give an HDF5 no file name error.
"vtkAMRFlashParticlesReader",
"vtkAMREnzoParticlesReader",
"vtkAMRFlashReader"
])

classLinuxExceptions = set([
"vtkAMREnzoReader" # core dump
])

# In the case of Windows, these classes cause a crash.
classWindowsExceptions = set([
"vtkWin32VideoSource", # Update() works the first time but a subsequent run calls up the webcam which crashes on close.

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

# Keep a record of the classes tested.
nonexistentClasses = set()
abstractClasses = set()
noConcreteImplementation = set()
noUpdate = set()
noShutdown = set()
noSetInput = set()
noObserver = set()
emptyInputWorked = set()
emptyInputFailed = set()

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
            result = re.match(regEx,repr(obj))
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
    except AttributeError as err:
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
    except AttributeError as err:
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
    except AttributeError as err:
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

    :param: cname - the name of the class to be tested.
    :return: True if at least one of the tests is successful, False otherwise.
    '''
    try:
        b = eval("vtk."+cname+"()")
        e = ErrorObserver()
        # A record of whether b.Update() worked or not.
        # The indexing corresponds to:
        # [vtkAlgorithm, emptyPD, emptyID, emptySG, emptyUG, emptyRG]
        updateStatus = [False, False, False, False, False, False]
        try:
            b.AddObserver('ErrorEvent', e)
        except AttributeError as err:
            noObserver.add(cname)
            return False
        except:
            raise
        if b.IsA("vtkAlgorithm"):
            u = TryUpdate(b,e)
            if u:
                updateStatus[0] = True
            else:
                updateStatus[0] = False
        s = TrySetInputData(b, e, emptyPD)
        if s:
            u = TryUpdate(b,e)
            if u:
                updateStatus[1] = True
            else:
                updateStatus[1] = False
        s = TrySetInputData(b, e, emptyID)
        if s:
            u = TryUpdate(b,e)
            if u:
                updateStatus[2] = True
            else:
                updateStatus[2] = False
        s = TrySetInputData(b, e, emptySG)
        if s:
            u = TryUpdate(b,e)
            if u:
                updateStatus[3] = True
            else:
                updateStatus[3] = False
        s = TrySetInputData(b, e, emptyUG)
        if s:
            u = TryUpdate(b,e)
            if u:
                updateStatus[4] = True
            else:
                updateStatus[4] = False
        s = TrySetInputData(b, e, emptyRG)
        if s:
            u = TryUpdate(b,e)
            if u:
                updateStatus[5] = True
            else:
                updateStatus[5] = False
        # If thread creation moves away from the vtkGeoSource constructor, then
        # this ShutDown call will not be necessary...
        #
        if b.IsA("vtkGeoSource"):
            TryShutdown(b, e)

        ok = False
        for value in updateStatus:
            ok |= value

        b = None
        return ok
    except TypeError as err:
        # Trapping abstract classes.
        abstractClasses.add(cname)
        return False
    except NotImplementedError as err:
        # No concrete implementation
        noConcreteImplementation.add(cname)
        return False
    except AttributeError as err:
        # Class does not exist
        nonexistentClasses.add(cname)
        return False
    except:
        raise

def TestEmptyInputTest(batch, batchNo = 0, batchSize = 0):
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
        res = " Testing -- {:4d} - {:s}".format(idx,a)
        if ( batchIdx < len(batch) - 1):
            nextRes = " Next -- {:4d} - {:s}".format(idx + 1,list(batch)[batchIdx +1])
        else:
            nextRes = "No next"
#         if verbose:
#             print res, nextRes
        ok =  TestOne(a)
        if ok:
            emptyInputWorked.add(a)
            if verbose:
                print res + ' - Ok'
        else:
            emptyInputFailed.add(a)
            if verbose:
                print res + ' - Fail'
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
                TestEmptyInputTest(batch, batchNo, batchSize)
                print total
                batch = set()
                total = 0
        idx += 1
    if batch:
        TestEmptyInputTest(batch, batchNo, batchSize)
        print total

def PrintResultSummary():
    print 'CTEST_FULL_OUTPUT (Avoid ctest truncation of output)'
    print 'EmptyInput worked: ', len(emptyInputWorked)
    print 'EmptyInput failed: ', len(emptyInputFailed)
    print '   Included in EmptyInput failed'
    print '   are the following:'
    print '\tAbstract classes: ', len(abstractClasses)
    print '\tNon-existent classes: ', len(nonexistentClasses)
    print '\tNo concrete implementation: ', len(noConcreteImplementation)
    print '\tNo observer could be added: ', len(noObserver)
    print '\tNo Update(): ', len(noUpdate)
    print '\tNo SetInput():', len(noSetInput)
    print '\tNo Shutdown(): ', len(noShutdown)
    print 'Summary:'
    print 'Not Tested: ', len(classExceptions)
    print 'Total number of classes tested: ', len(emptyInputWorked) + len(emptyInputFailed)
    print 'Total number of classes: ', len(emptyInputWorked) + len(emptyInputFailed) + len(classExceptions)


def ProgramOptions():
    desc = """
    %prog Tests for empty input for various data structures.
    """
    parser = optparse.OptionParser(description=desc)
    parser.set_defaults(verbose=False)

    parser.add_option('-c','--classnames',
        help='The name of the class or a list of classes in quotes separated by commas.',
        type='string',
        dest='classnames',
        default=None,
        action='store')
    parser.add_option('-q','--quiet',
        help='Do not print status messages to stdout (default)',
        dest='verbose',
        action="store_false")
    parser.add_option('-v','--verbose',
        help='Print status messages to stdout',
        dest='verbose',
        action="store_true")

    (opts,args) = parser.parse_args()

    return (True, opts)

def main(argv=None):
    global classExceptions
    global vtkClasses
    global classNames
    global verbose

    if argv is None:
        argv = sys.argv

    (res,opts) = ProgramOptions()
    if opts.classnames:
        cn = [x.strip() for x in opts.classnames.split(',')]
        classNames = set(cn)
    if opts.verbose:
        verbose = opts.verbose

    #RedirectVTKMessages()
    if classNames:
        TestEmptyInputTest(classNames)
    else:
        classExceptions = commonExceptions.union(classLinuxExceptions)
        classExceptions = classExceptions.union(classWindowsExceptions)

        vtkClasses = GetVTKClasses()
#         filter = ['Reader', 'Writer', 'Array_I', 'Qt']
#         vtkClasses = FilterClasses(vtkClasses, filter)
        vtkClasses = vtkClasses - classExceptions
        TestEmptyInputTest(vtkClasses)
#         In Windows
#         0-10, 10-17, 17-18, 18-23 in steps of 100 work but not if called
#         in a loop.
#         intervals = [[0,10]]# [[0,10]], [10,17], [17,18], [18,20]]
#         for j in intervals:
#             for i in range(j[0],j[1]):
#                 BatchTest(vtkClasses, i, 100)

    PrintResultSummary()
    print 'Finished.'

if __name__ == '__main__':
    sys.exit(main())
