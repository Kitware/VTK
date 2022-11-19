#!/usr/bin/env python

"""
vtkEnSightWriter test for writing node/element variables

This test demonstrates that vtkEnSightWriter supports writing
"scalar", "vector", "tensor symm" and "tensor asym" variables for
"per node" and "per cell" locations, and vtkEnSightGoldBinaryReader
is able to read them back.

Values of the created variables have a regular structure, eg. "PointVectors"
has values (1001, 1002, 1003), (2001, 2002, 2003)... for easier debugging.

"""

import vtk
from vtk.util.misc import vtkGetTempDir
import os.path
VTK_TEMP_DIR = vtkGetTempDir()

# Create some geometry
cell_type_source = vtk.vtkCellTypeSource()
cell_type_source.SetCellType(vtk.VTK_HEXAHEDRON)
cell_type_source.SetBlocksDimensions(2, 2, 2)
cell_type_source.Update()
reference_mesh = cell_type_source.GetOutput()
num_points = reference_mesh.GetNumberOfPoints()
num_cells = reference_mesh.GetNumberOfCells()

# Create testing variables
def fill_array(arr):
    for i in range(arr.GetNumberOfTuples()):
        arr.SetTuple(i, [1000*(i+1) + (j+1) for j in range(arr.GetNumberOfComponents())])

for dimension, name in [(1, "PointScalars"), (3, "PointVectors"), (6, "PointTensors6"), (9, "PointTensors9")]:
    arr = vtk.vtkFloatArray()
    arr.SetName(name)
    arr.SetNumberOfComponents(dimension)
    arr.SetNumberOfTuples(num_points)
    fill_array(arr)
    reference_mesh.GetPointData().AddArray(arr)

for dimension, name in [(1, "CellScalars"), (3, "CellVectors"), (6, "CellTensors6"), (9, "CellTensors9")]:
    arr = vtk.vtkFloatArray()
    arr.SetName(name)
    arr.SetNumberOfComponents(dimension)
    arr.SetNumberOfTuples(num_cells)
    fill_array(arr)
    reference_mesh.GetCellData().AddArray(arr)

# Create auxiliary variables to make sure we're mapping points/cells correctly to reference data.
# Note that we unfortunately cannot use vtkEnSightWriter WriteNodeIDs/WriteElementIDs since
# vtkEnSightGoldBinaryReader doesn't provide access to these arrays.
point_ids = vtk.vtkFloatArray()
point_ids.SetName("VTKPointIDs")
point_ids.SetNumberOfComponents(1)
point_ids.SetNumberOfTuples(num_points)
for i in range(num_points):
    point_ids.SetValue(i, i)
reference_mesh.GetPointData().AddArray(point_ids)

cell_ids = vtk.vtkFloatArray()
cell_ids.SetName("VTKCellIDs")
cell_ids.SetNumberOfComponents(1)
cell_ids.SetNumberOfTuples(num_points)
for i in range(num_cells):
    cell_ids.SetValue(i, i)
reference_mesh.GetCellData().AddArray(cell_ids)

# Write the case and read it back
writer = vtk.vtkEnSightWriter()
writer.SetFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterVariables.case"))
writer.SetInputData(reference_mesh)
writer.Write()
writer.WriteCaseFile(0)

reader = vtk.vtkEnSightGoldBinaryReader()
reader.SetCaseFileName(os.path.join(VTK_TEMP_DIR, "ensightWriterVariables.0.case"))
reader.ReadAllVariablesOn()
reader.Update()
mesh = reader.GetOutput().GetBlock(0)

# vtkEnSightWriter appends "_n" or "_c" suffix to the names
point_array_names = [
    ("PointScalars", "PointScalars_n"),
    ("PointVectors", "PointVectors_n"),
    ("PointTensors6", "PointTensors6_n"),
    ("PointTensors9", "PointTensors9_n"),
]

cell_array_names = [
    ("CellScalars", "CellScalars_c"),
    ("CellVectors", "CellVectors_c"),
    ("CellTensors6", "CellTensors6_c"),
    ("CellTensors9", "CellTensors9_c"),
]

point_id_to_reference = {i: int(mesh.GetPointData().GetArray("VTKPointIDs_n").GetValue(i)) for i in range(num_points)}
cell_id_to_reference = {i: int(mesh.GetCellData().GetArray("VTKCellIDs_c").GetValue(i)) for i in range(num_cells)}

# Check point variables
for reference_name, name in point_array_names:
    reference_arr = reference_mesh.GetPointData().GetArray(reference_name)
    arr = mesh.GetPointData().GetArray(name)
    assert reference_arr is not None
    assert arr is not None

    for i in range(num_points):
        j = point_id_to_reference[i]
        reference_val = reference_arr.GetTuple(i)
        val = arr.GetTuple(j)
        assert reference_val == val

# Check cell variables
for reference_name, name in cell_array_names:
    reference_arr = reference_mesh.GetCellData().GetArray(reference_name)
    arr = mesh.GetCellData().GetArray(name)
    assert reference_arr is not None
    assert arr is not None

    for i in range(num_cells):
        j = cell_id_to_reference[i]
        reference_val = reference_arr.GetTuple(i)
        val = arr.GetTuple(j)
        assert reference_val == val
