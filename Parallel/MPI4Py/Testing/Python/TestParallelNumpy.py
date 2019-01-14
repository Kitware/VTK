""" Test for various numpy_interface modules. Main goal is to test
parallel algorithms in vtk.numpy_interface.algorithms."""

from __future__ import print_function

import sys
try:
    import numpy
except ImportError:
    print("Numpy (http://numpy.scipy.org) not found.", end=' ')
    print("This test requires numpy!")
    from vtk.test import Testing
    Testing.skip()

import vtk
import vtk.numpy_interface.dataset_adapter as dsa
import vtk.numpy_interface.algorithms as algs
from mpi4py import MPI

c = vtk.vtkMPIController()
#c.SetGlobalController(None)
rank = c.GetLocalProcessId()
size = c.GetNumberOfProcesses()

def PRINT(text, values):
    if values is dsa.NoneArray:
        values = numpy.array(0, dtype=numpy.float64)
    else:
        values = numpy.array(numpy.sum(values)).astype(numpy.float64)
    res = numpy.array(values)
    MPI.COMM_WORLD.Allreduce([values, MPI.DOUBLE], [res, MPI.DOUBLE], MPI.SUM)
    assert numpy.abs(res) < 1E-5
    if rank == 0:
        print(text, res)

def testArrays(rtData, rtData2, grad, grad2, total_npts):
    " Test various parallel algorithms."
    if rank == 0:
        print('-----------------------')
    PRINT( "SUM ones:", algs.sum(rtData / rtData) - total_npts )

    PRINT( "SUM sin:", (algs.sum(algs.sin(rtData) + 1) - numpy.sum(numpy.sin(rtData2) + 1)) / numpy.sum(numpy.sin(rtData2) + 1) )

    PRINT( "rtData min:", algs.min(rtData) - numpy.min(rtData2) )
    PRINT( "rtData max:", algs.max(rtData) - numpy.max(rtData2) )
    PRINT( "rtData sum:", (algs.sum(rtData) - numpy.sum(rtData2)) / (2*numpy.sum(rtData2)) )
    PRINT( "rtData mean:", (algs.mean(rtData) - numpy.mean(rtData2)) / (2*numpy.mean(rtData2)) )
    PRINT( "rtData var:", (algs.var(rtData) - numpy.var(rtData2)) / numpy.var(rtData2) )
    PRINT( "rtData std:", (algs.std(rtData) - numpy.std(rtData2)) / numpy.std(rtData2) )

    PRINT( "grad min:", algs.min(grad) - numpy.min(grad2) )
    PRINT( "grad max:", algs.max(grad) - numpy.max(grad2) )
    PRINT( "grad min 0:", algs.min(grad, 0) - numpy.min(grad2, 0) )
    PRINT( "grad max 0:", algs.max(grad, 0) - numpy.max(grad2, 0) )
    PRINT( "grad min 1:", algs.sum(algs.min(grad, 1)) - numpy.sum(numpy.min(grad2, 1)) )
    PRINT( "grad max 1:", algs.sum(algs.max(grad, 1)) - numpy.sum(numpy.max(grad2, 1)) )
    PRINT( "grad sum 1:", algs.sum(algs.sum(grad, 1)) - numpy.sum(numpy.sum(grad2, 1)) )
    PRINT( "grad var:", (algs.var(grad) - numpy.var(grad2)) / numpy.var(grad2) )
    PRINT( "grad var 0:", (algs.var(grad, 0) - numpy.var(grad2, 0)) / numpy.var(grad2, 0) )

w = vtk.vtkRTAnalyticSource()
# Update with ghost level because gradient needs it
# to be piece independent
w.UpdatePiece(rank, size, 1)

print(w.GetOutput())
print(w.GetOutputInformation(0))

# The parallel arrays that we care about
ds = dsa.WrapDataObject(w.GetOutput())
rtData = ds.PointData['RTData']
grad = algs.gradient(rtData)
ds.PointData.append(grad, 'gradient')

# Crop the any ghost points out
org_ext = w.GetOutput().GetExtent()
ext = list(org_ext)
wext = w.GetOutputInformation(0).Get(vtk.vtkStreamingDemandDrivenPipeline.WHOLE_EXTENT())
for i in range(3):
    if ext[2*i] != wext[2*i]:
        ext[2*i] = ext[2*i] + 2
    if ext[2*i+1] != wext[2*i+1]:
        ext[2*i+1] = ext[2*i+1] - 1
if ext != list(org_ext):
    w.GetOutput().Crop(ext)

# Croppped arrays
rtData = ds.PointData['RTData']
grad = ds.PointData['gradient']

# The whole dataset so that we can compare
# against parallel algorithms.
w2 = vtk.vtkRTAnalyticSource()
w2.Update()

ds2 = dsa.WrapDataObject(w2.GetOutput())
rtData2 = ds2.PointData['RTData']
grad2 = algs.gradient(rtData2)

npts = numpy.array(numpy.int32(ds.GetNumberOfPoints()))
total_npts = numpy.array(npts)
MPI.COMM_WORLD.Allreduce([npts, MPI.INT], [total_npts, MPI.INT], MPI.SUM)

# Test simple distributed data.
testArrays(rtData, rtData2, grad, grad2, total_npts)

# Check that we can disable parallelism by using a dummy controller
# even when a global controller is set
assert algs.sum(rtData / rtData, controller=vtk.vtkDummyController()) != total_npts

# Test where arrays are NoneArray on one of the ranks.
if size > 1:
    if rank == 0:
        rtData3 = rtData2
        grad3 = grad2
    else:
        rtData3 = dsa.NoneArray
        grad3 = dsa.NoneArray

    testArrays(rtData3, rtData2, grad3, grad2, total_npts)

# Test composite arrays
rtData3 = dsa.VTKCompositeDataArray([rtData, dsa.NoneArray])
grad3 = dsa.VTKCompositeDataArray([dsa.NoneArray, grad])

testArrays(rtData3, rtData2, grad3, grad2, total_npts)

# Test where arrays are NoneArray on one of the ranks
# and composite on others.
if size > 1:
    if rank == 1:
        rtData3 = dsa.VTKCompositeDataArray([rtData2])
        grad3 = dsa.VTKCompositeDataArray([grad2])
    else:
        rtData3 = dsa.NoneArray
        grad3 = dsa.NoneArray

    testArrays(rtData3, rtData2, grad3, grad2, total_npts)

# Test composite arrays with multiple blocks.

# Split the local image to 2.
datasets = []
for i in range(2):
    image = vtk.vtkImageData()
    image.ShallowCopy(w.GetOutput())
    t = vtk.vtkExtentTranslator()
    wext = image.GetExtent()
    t.SetWholeExtent(wext)
    t.SetPiece(i)
    t.SetNumberOfPieces(2)
    t.PieceToExtent()
    ext = list(t.GetExtent())

    # Crop the any ghost points out
    for i in range(3):
        if ext[2*i] != wext[2*i]:
            ext[2*i] = ext[2*i] + 1
    if ext != list(org_ext):
        image.Crop(ext)

    datasets.append(dsa.WrapDataObject(image))

rtData3 = dsa.VTKCompositeDataArray([datasets[0].PointData['RTData'], datasets[1].PointData['RTData']])
grad3 = dsa.VTKCompositeDataArray([datasets[0].PointData['gradient'], datasets[1].PointData['gradient']])

testArrays(rtData3, rtData2, grad3, grad2, total_npts)

# Test min/max per block
NUM_BLOCKS = 10

w = vtk.vtkRTAnalyticSource()
w.SetWholeExtent(0, 10, 0, 10, 0, 10)
w.Update()

c = vtk.vtkMultiBlockDataSet()
c.SetNumberOfBlocks(size*NUM_BLOCKS)

if rank == 0:
    start = 0
    end = NUM_BLOCKS
else:
    start = rank*NUM_BLOCKS - 3
    end = start + NUM_BLOCKS

for i in range(start, end):
    a = vtk.vtkImageData()
    a.ShallowCopy(w.GetOutput())
    c.SetBlock(i, a)

if rank == 0:
    c.SetBlock(NUM_BLOCKS - 1, vtk.vtkPolyData())

cdata = dsa.WrapDataObject(c)
rtdata = cdata.PointData['RTData']
rtdata = algs.abs(rtdata)
g = algs.gradient(rtdata)
g2 = algs.gradient(g)

res = True
dummy = vtk.vtkDummyController()
for axis in [None, 0]:
    for array in [rtdata, g, g2]:
        if rank == 0:
            array2 = array/2
            min = algs.min_per_block(array2, axis=axis)
            res &= numpy.all(min.Arrays[NUM_BLOCKS - 1] == numpy.min(array, axis=axis))
            all_min = algs.min(min, controller=dummy)
            all_min_true = numpy.min([algs.min(array, controller=dummy), algs.min(array2, controller=dummy)])
            res &= all_min == all_min_true
            max = algs.max_per_block(array2, axis=axis)
            res &= numpy.all(max.Arrays[NUM_BLOCKS - 1] == numpy.max(array, axis=axis))
            all_max = algs.max(max, controller=dummy)
            all_max_true = numpy.max([algs.max(array, controller=dummy), algs.max(array2, controller=dummy)])
            res &= all_max == all_max_true
            sum = algs.sum_per_block(array2, axis=axis)
            sum_true = numpy.sum(array2.Arrays[0]) * (NUM_BLOCKS-1)
            sum_true += numpy.sum(array.Arrays[0]) * 3
            res &= numpy.sum(algs.sum(sum, controller=dummy) - algs.sum(sum_true, controller=dummy)) == 0
            mean = algs.mean_per_block(array2, axis=axis)
            res &= numpy.sum(mean.Arrays[0] - numpy.mean(array2.Arrays[0], axis=axis)) < 1E-6
            if len(array.Arrays[0].shape) == 1:
                stk = numpy.hstack
            else:
                stk = numpy.vstack
            res &= numpy.sum(mean.Arrays[NUM_BLOCKS-2] - numpy.mean(stk((array.Arrays[0], array2.Arrays[0])), axis=axis)) < 1E-4
        elif rank == 2:
            min = algs.min_per_block(dsa.NoneArray, axis=axis)
            max = algs.max_per_block(dsa.NoneArray, axis=axis)
            sum = algs.sum_per_block(dsa.NoneArray, axis=axis)
            mean = algs.mean_per_block(dsa.NoneArray, axis=axis)
        else:
            min = algs.min_per_block(array, axis=axis)
            max = algs.max_per_block(array, axis=axis)
            sum = algs.sum_per_block(array, axis=axis)
            mean = algs.mean_per_block(array, axis=axis)
        if array is g and axis == 0:
            ug = algs.unstructured_from_composite_arrays(mean, [(mean, 'mean')])
            if mean is dsa.NoneArray:
                res &= ug.GetNumberOfPoints() == 0
            else:
                _array = ug.GetPointData().GetArray('mean')
                ntuples = _array.GetNumberOfTuples()
                for i in range(ntuples):
                    if rank == 1:
                        idx = i+3
                    else:
                        idx = i
                    res &= _array.GetTuple(i) == tuple(mean.Arrays[idx])

res &= algs.min_per_block(dsa.NoneArray) is dsa.NoneArray

if rank == 0:
    min = algs.min_per_block(rtdata.Arrays[0]/2)
elif rank == 2:
    min = algs.min_per_block(dsa.NoneArray)
    res &= min is dsa.NoneArray
else:
    min = algs.min_per_block(rtdata.Arrays[0])

if rank == 0:
    min = algs.min(rtdata.Arrays[0])
    res &= min == numpy.min(rtdata.Arrays[0])
else:
    min = algs.min(dsa.NoneArray)
    res &= min is dsa.NoneArray

res &= algs.min(dsa.NoneArray) is dsa.NoneArray

if rank == 0:
    res &= numpy.all(algs.min(g2, axis=0) == numpy.min(g2.Arrays[0], axis=0))
else:
    res &= algs.min(dsa.NoneArray, axis=0) is dsa.NoneArray

res = numpy.array(res, dtype=numpy.bool)
all_res = numpy.array(res)
mpitype = algs._lookup_mpi_type(numpy.bool)
MPI.COMM_WORLD.Allreduce([res, mpitype], [all_res, mpitype], MPI.LAND)
assert all_res
