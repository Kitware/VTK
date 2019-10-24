#!/usr/bin/env python
# -*- coding: utf-8 -*-

import vtk
import sys

# Test bounds computation for mixed cells in polydata.
# Create some points that are unused by any cell as well.
polyData = vtk.vtkPolyData()
pts = vtk.vtkPoints()
verts = vtk.vtkCellArray()
lines = vtk.vtkCellArray()
polys = vtk.vtkCellArray()
strips = vtk.vtkCellArray()

pts.SetNumberOfPoints(13)
pts.SetPoint(0, 0,0,0)
pts.SetPoint(1, 1,0,0)
pts.SetPoint(2, 2,0,0)
pts.SetPoint(3, 3,0,0)
pts.SetPoint(4, 4,0,0)
pts.SetPoint(5, 3,1,0)
pts.SetPoint(6, 4,1,0)
pts.SetPoint(7, 5,0,0)
pts.SetPoint(8, 6,0,0)
pts.SetPoint(9, 5,1,0)
pts.SetPoint(10, 6,1,0)
pts.SetPoint(11, 7,0,0)
pts.SetPoint(12, 8,0,0)

verts.InsertNextCell(1)
verts.InsertCellPoint(0)

lines.InsertNextCell(2)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)

polys.InsertNextCell(4)
polys.InsertCellPoint(3)
polys.InsertCellPoint(4)
polys.InsertCellPoint(6)
polys.InsertCellPoint(5)

strips.InsertNextCell(4)
strips.InsertCellPoint(7)
strips.InsertCellPoint(8)
strips.InsertCellPoint(9)
strips.InsertCellPoint(10)

polyData.SetPoints(pts)
polyData.SetVerts(verts)
polyData.SetLines(lines)
polyData.SetPolys(polys)
polyData.SetStrips(strips)

box = [0.0,0.0,0.0,0.0,0.0,0.0]

print("Input data:")
print("\tNum Points: {0}".format(polyData.GetNumberOfPoints()))
print("\tNum Cells: {0}".format(polyData.GetNumberOfCells()))

# Currently vtkPolyData takes into account cells that are connected to
# points; hence only connected points (i.e., points used by cells) are
# considered.

# Compute bounds on polydata
polyData.GetBounds(box)

assert box[0] == 0.0
assert box[1] == 6.0
assert box[2] == 0.0
assert box[3] == 1.0
assert box[4] == 0.0
assert box[5] == 0.0
