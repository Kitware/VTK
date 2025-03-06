from __future__ import print_function

from vtkmodules.vtkFiltersParallel import vtkCleanArrays
from vtkmodules.vtkParallelCore import vtkMultiProcessController
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkCommonDataModel import *
from vtkmodules.vtkCommonCore import *

cntrl = vtkMultiProcessController.GetGlobalController()
rank = cntrl.GetLocalProcessId()
numprocs = cntrl.GetNumberOfProcesses()

#-----------------------------------------------------------------------------
if rank == 0:
    print("Testing on non-composite dataset")

def get_dataset(pa=None,ca=None):
    sphere = vtkSphereSource()
    sphere.Update()

    data = sphere.GetOutputDataObject(0)
    data.GetPointData().Initialize()
    data.GetCellData().Initialize()

    if pa:
        array = vtkIntArray()
        array.SetName(pa)
        array.SetNumberOfTuples(data.GetNumberOfPoints())
        data.GetPointData().AddArray(array)

    if ca:
        array = vtkIntArray()
        array.SetName(ca)
        array.SetNumberOfTuples(data.GetNumberOfCells())
        data.GetCellData().AddArray(array)
    return data


data = get_dataset("PD-%d" % rank, "CD-%d" % rank)
cleanArrays = vtkCleanArrays()
cleanArrays.SetInputDataObject(data)
cleanArrays.SetController(cntrl)

# Test removing partial arrays.
cleanArrays.FillPartialArraysOff()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)

# Each rank tests the next ranks data.
if numprocs > 1:
    assert result.GetPointData().GetArray("PD-%d" % ((rank+1)%numprocs)) is None and \
            result.GetCellData().GetArray("CD-%d" % ((rank+1)%numprocs)) is None

# Test filling partial arrays.
cleanArrays.FillPartialArraysOn()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)

# Each rank tests the next ranks data.
assert result.GetPointData().GetNumberOfArrays() == numprocs and \
        result.GetCellData().GetNumberOfArrays() == numprocs

assert result.GetPointData().GetArray("PD-%d" % ((rank+1)%numprocs)) is not None and \
        result.GetCellData().GetArray("CD-%d" % ((rank+1)%numprocs)) is not None


#-----------------------------------------------------------------------------
if rank == 0:
    print("Testing on composite dataset")


#-----------------------------------------------------------------------------
# Dataset with identical arrays for non-empty datasets on all ranks.
mb = vtkMultiBlockDataSet()
mb.SetNumberOfBlocks(numprocs)
mb.SetBlock(rank, get_dataset(pa="pa", ca="ca"))

cleanArrays.SetInputDataObject(mb)
cleanArrays.FillPartialArraysOff()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)
assert result.GetBlock(rank).GetPointData().GetNumberOfArrays() == 1 and \
        result.GetBlock(rank).GetCellData().GetNumberOfArrays() == 1

cleanArrays.FillPartialArraysOn()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)
assert result.GetBlock(rank).GetPointData().GetNumberOfArrays() == 1 and \
        result.GetBlock(rank).GetCellData().GetNumberOfArrays() == 1

#-----------------------------------------------------------------------------
# Dataset with partial arrays for non-empty datasets on all ranks.
mb = vtkMultiBlockDataSet()
mb.SetNumberOfBlocks(2*numprocs)
mb.SetBlock(rank, get_dataset(pa="pa-%d" % rank, ca="ca-%d" % rank))
# Let's add an extra block with new arrays so the test can work even when
# numprocs == 1.
mb.SetBlock(numprocs + rank, get_dataset(pa="pa", ca="ca"))

cleanArrays.SetInputDataObject(mb)
cleanArrays.FillPartialArraysOff()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)
assert result.GetBlock(rank).GetPointData().GetNumberOfArrays() == 0 and \
        result.GetBlock(rank).GetCellData().GetNumberOfArrays() == 0

cleanArrays.FillPartialArraysOn()
cleanArrays.Update()
result = cleanArrays.GetOutputDataObject(0)
assert result.GetBlock(rank).GetPointData().GetNumberOfArrays() == (numprocs+1) and \
        result.GetBlock(rank).GetCellData().GetNumberOfArrays() == (numprocs+1)
print("%d-Passed!" % rank)
