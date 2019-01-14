#!/usr/bin/env python
import sys
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def Gather(c, arr, root):
    vtkArr = vtk.vtkDoubleArray()
    count = len(arr)
    vtkArr.SetNumberOfTuples(count)
    for i in range(count):
        vtkArr.SetValue(i, arr[i])
    vtkResult = vtk.vtkDoubleArray()
    c.Gather(vtkArr, vtkResult, root)
    result = [vtkResult.GetValue(i) for i in range(vtkResult.GetNumberOfTuples())]
    return [ tuple(result[i : i + count]) \
                for i in range(0, vtkResult.GetNumberOfTuples(), count) ]

renWin = vtk.vtkRenderWindow()

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

r = vtk.vtkMultiBlockPLOT3DReader()
# Since vtkMPIMultiBlockPLOT3DReader is not created on Windows even when MPI
# is enabled.
assert r.IsA("vtkMPIMultiBlockPLOT3DReader") == 1 or sys.platform == "win32"

r.SetFileName(VTK_DATA_ROOT + "/Data/multi-bin.xyz")
r.SetQFileName(VTK_DATA_ROOT + "/Data/multi-bin-oflow.q")
r.SetFunctionFileName(VTK_DATA_ROOT + "/Data/multi-bin.f")
r.AutoDetectFormatOn()

r.Update()

c = vtk.vtkMPIController.GetGlobalController()
size = c.GetNumberOfProcesses()
rank = c.GetLocalProcessId()
block = 0

bounds = r.GetOutput().GetBlock(block).GetBounds()
bounds = Gather(c, bounds, root=0)

if rank == 0:
    print("Reader:", r.GetClassName())
    print("Bounds:")
    for i in range(size):
        print(bounds[i])

c.Barrier()
aname = "StagnationEnergy"
rng = r.GetOutput().GetBlock(block).GetPointData().GetArray(aname).GetRange(0)

rng = Gather(c, rng, root=0)
if rank == 0:
    print("StagnationEnergy Ranges:")
    for i in range(size):
        print(rng[i])
        assert rng[i][0] > 1.1 and rng[i][0] < 24.1 and \
               rng[i][1] > 1.1 and rng[i][1] < 24.1
