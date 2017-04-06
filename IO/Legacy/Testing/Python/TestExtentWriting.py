 #!/usr/bin/env python

import os
import vtk

#
# If the current directory is writable, then test the witers
#
try:
    channel = open("test.tmp", "w")
    channel.close()
    os.remove("test.tmp")

    s = vtk.vtkRTAnalyticSource()
    s.SetWholeExtent(5, 10, 5, 10, 5, 10)
    s.Update()

    d = s.GetOutput()

    w = vtk.vtkStructuredPointsWriter()
    w.SetInputData(d)
    w.SetFileName("test-dim.vtk")
    w.Write()

    r = vtk.vtkStructuredPointsReader()
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

    rg = vtk.vtkRectilinearGrid()
    extents = (1, 3, 1, 3, 1, 3)
    rg.SetExtent(extents)
    pts = vtk.vtkFloatArray()
    pts.InsertNextTuple1(0)
    pts.InsertNextTuple1(1)
    pts.InsertNextTuple1(2)
    rg.SetXCoordinates(pts)
    rg.SetYCoordinates(pts)
    rg.SetZCoordinates(pts)

    w = vtk.vtkRectilinearGridWriter()
    w.SetInputData(rg)
    w.SetFileName("test-dim.vtk")
    w.Write()

    r = vtk.vtkRectilinearGridReader()
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

    sg = vtk.vtkStructuredGrid()
    extents = (1, 3, 1, 3, 1, 3)
    sg.SetExtent(extents)
    ptsa = vtk.vtkFloatArray()
    ptsa.SetNumberOfComponents(3)
    ptsa.SetNumberOfTuples(27)
    # We don't really care about point coordinates being correct
    for i in range(27):
        ptsa.InsertNextTuple3(0, 0, 0)
    pts = vtk.vtkPoints()
    pts.SetData(ptsa)
    sg.SetPoints(pts)

    w = vtk.vtkStructuredGridWriter()
    w.SetInputData(sg)
    w.SetFileName("test-dim.vtk")
    w.Write()

    r = vtk.vtkStructuredGridReader()
    r.SetFileName("test-dim.vtk")
    r.Update()

    os.remove("test-dim.vtk")

    assert(r.GetOutput().GetExtent() == (0,2,0,2,0,2))

    w.SetInputData(sg)
    w.SetFileName("test-dim.vtk")
    w.SetWriteExtent(True)
    w.Write()

    r.Modified()
    r.Update()

    assert(r.GetOutput().GetExtent() == (1,3,1,3,1,3))


except IOError:
    pass
