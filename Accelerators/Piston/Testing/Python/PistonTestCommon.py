"""
Common utilities for all of the vtkPiston python tests.

"""

import sys
import vtk

def parseArgs():
    argv = sys.argv

    result = {};
    stripped_args = []
    for i in range(0, len(argv)):
        if argv[i] == '--save_data':
            result["SaveData"] = True
        elif argv[i] == '--gpu_render':
            result["GPURender"] = True
        elif argv[i] == '-I':
            result["Interactive"] = True
            stripped_args.append(argv[i])
        elif argv[i] == '--normalize':
            result["Normalize"] = True
        else:
            stripped_args.append(argv[i])
    sys.argv = stripped_args
    return result

def printDS(label, id):
    print label
    print id.__this__
    print id.GetClassName()
    print id.GetBounds()
    print id.GetNumberOfPoints()
    print id.GetNumberOfCells()
    numarrays = id.GetPointData().GetNumberOfArrays()
    print "Number of Point arrays", numarrays
    for x in range(numarrays):
        na = id.GetPointData().GetArray(x)
        print na.GetName()
        print na.GetDataType()
        print na.GetNumberOfComponents()
        print na.GetNumberOfTuples()

def printTDO(label, id):
    print label
    print id.__this__
    print id.GetClassName()
    print id.GetReferredType()
    print id.GetReferredData()

def writeFile(ifilter, filename):
    dsw = vtk.vtkDataSetWriter()
    dsw.SetInputConnection(ifilter.GetOutputPort())
    dsw.SetFileName(filename)
    dsw.Write()
