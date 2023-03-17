#!/usr/bin/env python

"""
vtkEnSightWriter test for writing VTK_CONVEX_POINT_SET cell

vtkEnSightWriter should write VTK_CONVEX_POINT_SET as "nfaced" element
and vtkEnSightGoldBinaryReader will read it back as VTK_POLYHEDRON.
"""

import vtk
from vtk.util.misc import vtkGetDataRoot, vtkGetTempDir
import os.path
VTK_DATA_ROOT = vtkGetDataRoot()
VTK_TEMP_DIR = vtkGetTempDir()

cps = vtk.vtkConvexPointSet()
points = vtk.vtkPoints()
points.InsertNextPoint(0, 0, 0)
points.InsertNextPoint(1, 0, 0)
points.InsertNextPoint(1, 1, 0)
points.InsertNextPoint(0, 1, 0)
points.InsertNextPoint(0, 0, 1)
points.InsertNextPoint(1, 0, 1)
points.InsertNextPoint(1, 1, 1)
points.InsertNextPoint(0, 1, 1)
points.InsertNextPoint(0.5, 0, 0)
points.InsertNextPoint(1, 0.5, 0)
points.InsertNextPoint(0.5, 1, 0)
points.InsertNextPoint(0, 0.5, 0)
points.InsertNextPoint(0.5, 0.5, 0)

for i in range(13):
    cps.GetPointIds().InsertId(i, i)

reference_mesh = vtk.vtkUnstructuredGrid()
reference_mesh.Allocate(1, 1)
reference_mesh.InsertNextCell(cps.GetCellType(), cps.GetPointIds())
reference_mesh.SetPoints(points)

writer = vtk.vtkEnSightWriter()
writer.SetFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterConvexPointSet.case"))
writer.SetInputData(reference_mesh)
writer.Write()

reader = vtk.vtkEnSightGoldBinaryReader()
reader.SetCaseFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterConvexPointSet.0.case"))
reader.Update()
mesh = reader.GetOutput().GetBlock(0)

assert mesh.GetNumberOfPoints() == reference_mesh.GetNumberOfPoints()
assert mesh.GetNumberOfCells() == reference_mesh.GetNumberOfCells()
assert reference_mesh.GetCellType(0) == vtk.VTK_CONVEX_POINT_SET
assert mesh.GetCellType(0) == vtk.VTK_POLYHEDRON
