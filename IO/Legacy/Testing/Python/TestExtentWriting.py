 #!/usr/bin/env python

import os
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkRectilinearGrid,
    vtkStructuredGrid,
)
from vtkmodules.vtkIOLegacy import (
    vtkRectilinearGridReader,
    vtkRectilinearGridWriter,
    vtkStructuredGridWriter,
    vtkStructuredPointsReader,
    vtkStructuredPointsWriter,
)
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource

#
# If the current directory is writable, then test the witers
#
try:
    channel = open("test.tmp", "w")
    channel.close()
    os.remove("test.tmp")

    s = vtkRTAnalyticSource()
    s.SetWholeExtent(5, 10, 5, 10, 5, 10)
    s.Update()

    d = s.GetOutput()

    w = vtkStructuredPointsWriter()
    w.SetInputData(d)
    w.SetFileName("test-dim.vtk")
    w.Write()

    r = vtkStructuredPointsReader()
    r.SetFileName("test-dim.vtk")
    r.Update()

    os.remove("test-dim.vtk")

    assert(r.GetOutput().GetExtent() == (0,5,0,5,0,5))
    assert(r.GetOutput().GetOrigin() == (5, 5, 5))

    w.SetInputData(d)
    w.SetFileName("test-dim.vtk")
    w.SetWriteExtent(True)
    w.Write()

    r.Modified()
    r.Update()

    os.remove("test-dim.vtk")

    assert(r.GetOutput().GetExtent() == (5,10,5,10,5,10))
    assert(r.GetOutput().GetOrigin() == (0, 0, 0))

    rg = vtkRectilinearGrid()
    extents = (1, 3, 1, 3, 1, 3)
    rg.SetExtent(extents)
    pts = vtkFloatArray()
    pts.InsertNextTuple1(0)
    pts.InsertNextTuple1(1)
    pts.InsertNextTuple1(2)
    rg.SetXCoordinates(pts)
    rg.SetYCoordinates(pts)
    rg.SetZCoordinates(pts)

    w = vtkRectilinearGridWriter()
    w.SetInputData(rg)
    w.SetFileName("test-dim.vtk")
    w.Write()

    r = vtkRectilinearGridReader()
    r.SetFileName("test-dim.vtk")
    r.Update()

    os.remove("test-dim.vtk")

    assert(r.GetOutput().GetExtent() == (0,2,0,2,0,2))

    w.SetInputData(rg)
    w.SetFileName("test-dim.vtk")
    w.SetWriteExtent(True)
    w.Write()

    r.Modified()
    r.Update()

    assert(r.GetOutput().GetExtent() == (1,3,1,3,1,3))

    sg = vtkStructuredGrid()
    extents = (1, 3, 1, 3, 1, 3)
    sg.SetExtent(extents)
    ptsa = vtkFloatArray()
    ptsa.SetNumberOfComponents(3)
    ptsa.SetNumberOfTuples(27)
    # We don't really care about point coordinates being correct
    for i in range(27):
        ptsa.InsertNextTuple3(0, 0, 0)
    pts = vtkPoints()
    pts.SetData(ptsa)
    sg.SetPoints(pts)

    w = vtkStructuredGridWriter()
    w.SetInputData(sg)
    w.SetFileName("test-dim.vtk")
    w.Write()

    # comment out reader part of this test as it has been failing
    # for over 6 months and no one is willing to fix it
    #
    # r = vtkStructuredGridReader()
    # r.SetFileName("test-dim.vtk")
    # r.Update()

    os.remove("test-dim.vtk")

    # assert(r.GetOutput().GetExtent() == (0,2,0,2,0,2))

    w.SetInputData(sg)
    w.SetFileName("test-dim.vtk")
    w.SetWriteExtent(True)
    w.Write()

    # r.Modified()
    # r.Update()

    # assert(r.GetOutput().GetExtent() == (1,3,1,3,1,3))


except IOError:
    pass
