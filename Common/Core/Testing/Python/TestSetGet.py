#!/usr/bin/env python

import sys
import inspect
import re
import optparse

import vtk
from pydoc import classname

'''
   This is a translation from the TCL code of the same name.
'''
vtk.vtkObject.GlobalWarningDisplayOff()

def RedirectVTKMessages():
    '''
    Can be used to redirect VTK related error messages to a
    file.
    '''
    log = vtk.vtkFileOutputWindow()
    log.SetFlush(1)
    log.AppendOff()
    log.SetFileName('TestSetGet-err.log')
    log.SetInstance(log)


commonExceptions = set([
"vtkDistributedDataFilter",  # core dump
"vtkDataEncoder",  # Use after free error.
"vtkWebApplication",  # Thread issues - calls vtkDataEncoder
"vtkView",  # vtkView.SetRepresentation(None) fails

"vtkGenericAttributeCollection",  # Assert error
"vtkOverlappingAMR",

 # These give an HDF5 no file name error.
"vtkAMRFlashParticlesReader",
"vtkAMREnzoParticlesReader",
"vtkAMRFlashReader",

# core dump in release mode, issue is vtkChartMatrix.GetChart() & vtkScatterPlotMatrix.SetActivePlot()
"vtkChartMatrix",
"vtkScatterPlotMatrix"
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

# This will hold the classes to be tested.
vtkClasses = set()
classNames = None
classesTested = set()

# Keep a record of the classes tested.
nonexistentClasses = set()
abstractClasses = set()
noConcreteImplementation = set()
noObserver = set()
setGetWorked = set()
setGetFailed = set()

#------------------------
# These variables are used for further record keeping
# should users wish to investigate or debug further.
noGetSetPairs = set()
# These will be a list of the get/set functions keyed on class name.
getParameterFail = dict()
setParameterFail = dict()
#------------------------

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
    :return: a set of all the VTK classes that are e.g. Readers.
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

def TestOne(cname):
    '''
    Test the named class looking for complementary Set/Get pairs to test.

    The test is essentially this: Set(Get()).

    Some classes will generate a TypeError or an AttributeError,
    in this case, the name of the class is stored in the global variable
    abstractClasses or noConcreteImplementation with False being returned.

    If Set... or Get... fails a record of this is kept in the global classes:
    setParameterFail and getParameterFail

    Return values:
    0: If not any of the Set(Get()) tests work.
    1: If at least one of Set(Get()) tests works.
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
        try:
            b.AddObserver('ErrorEvent', e)
        except AttributeError:
            noObserver.add(cname)
            return 2
        except:
            raise

        getPattern = r'(^Set(.*)$)'
        getRegEx = re.compile(getPattern)
        setPattern = r'(^Get(.*)$)'
        setRegEx = re.compile(setPattern)

        methods = [method for method in dir(getattr(vtk, cname))
                    if callable(getattr(getattr(vtk, cname), method))]
        # Partition the Set/Get functions into separate sets.
        gets = set()
        sets = set()
        for m in methods:
            result = re.match(getRegEx, m)
            if result:
                gets.add(result.group(2))
            result = re.match(setRegEx, m)
            if result:
                sets.add(result.group(2))

        ok = False
        # These are our Set... Get... functions.
        matchingGetSet = set.intersection(gets, sets)
        setGetStatus = False
        if matchingGetSet:
            for m in set.intersection(gets, sets):
                # Get...() with no parameters.
                try:
                    x = getattr(b, 'Get' + m)()
                    try:
                        if type(x) is tuple:
                            x = list(x)
                            getattr(b, 'Set' + m)(*x)
                        else:
                            getattr(b, 'Set' + m)(x)
                        setGetStatus |= True
                    except TypeError:
                        # The parameter passed is of the wrong type
                        # or wrong number of parameters.
                        try:
                            value = setParameterFail[cname]
                            value.add('Set' + m)
                            setParameterFail[cname] = value
                        except KeyError:
                            # Key is not present
                            setParameterFail[cname] = set(['Set' + m])
                    except Exception as err:
                        print(cname + 'Set' + m + ' ' + str(err))
                except TypeError:
                    # Get...() takes 1 or more arguments.
                    try:
                        value = getParameterFail[cname]
                        value.add('Get' + m)
                        getParameterFail[cname] = value
                    except KeyError:
                        # Key is not present
                        getParameterFail[cname] = set(['Get' + m])
                except Exception as err:
                    print(cname + 'Get' + m + ' ' + str(err))
            ok = setGetStatus
        else:
            noGetSetPairs.add(cname)

        b = None
        if ok:
            setGetWorked.add(cname)
            return 1
        setGetFailed.add(cname)
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

def TestSetGet(batch, batchNo=0, batchSize=0):
    '''
    Test Set/Get pairs in each batch.

    If at least one of the Set(Get()) tests pass, the name of the
    class is added to the global variable setGetWorked otherwise
    to the global variable setGetFailed.

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
#        print(nextRes)

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
                TestSetGet(batch, batchNo, batchSize)
                batch = set()
                total = 0
        idx += 1
    if batch:
        TestSetGet(batch, batchNo, batchSize)

def PrintResultSummary():
    print('-' * 40)
    print('Set(Get()) worked: %i' % len(setGetWorked))
    print('Set(Get()) failed: %i' % len(setGetFailed))
    print('Abstract classes: %i' % len(abstractClasses))
    print('Non-existent classes: %i' % len(nonexistentClasses))
    print('No concrete implementation: %i' % len(noConcreteImplementation))
    print('No observer could be added: %i' % len(noObserver))
    print('-' * 40)
    print('Total number of classes tested: %i' % len(classesTested))
    print('-' * 40)
    print('Excluded from testing: %i' % len(classExceptions))
    print('-' * 40)

def ProgramOptions():
    desc = """
    %prog Tests each VTK class for Set(Get()) where Get() has no parameters.
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
        TestSetGet(classNames)
    else:
        classExceptions = commonExceptions.union(classLinuxExceptions)
        classExceptions = classExceptions.union(classWindowsExceptions)

        vtkClasses = GetVTKClasses()
#         filter = ['Reader', 'Writer', 'Array_I', 'Qt']
#         vtkClasses = FilterClasses(vtkClasses, filter)
        vtkClasses = vtkClasses - classExceptions
        TestSetGet(vtkClasses)
#         In Windows
#         0-10, 10-17, 17-18, 18-23 in steps of 100 work but not if called
#         in a loop.
#         intervals = [[0,10]]  # [[0,10]], [10,17], [17,18], [18,20]]
#         for j in intervals:
#             for i in range(j[0], j[1]):
#                 BatchTest(vtkClasses, i, 100)
#                 print(vtkClasses)

    PrintResultSummary()

if __name__ == '__main__':
    sys.exit(main())
