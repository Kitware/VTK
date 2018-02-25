#!/usr/bin/env python

"""
EnSight Gold Binary test with empty parts

The input case file contains 4 parts:

    part 1 - unstructured, 1 triangle
    part 2 - unstructured, no geometry
    part 3 - unstructured, no geometry
    part 4 - unstructured, 1 triangle

The non-empty parts define variables: "scalar_per_element",
"vector_per_element", "scalar_per_node", "vector_per_node".

This test ensures that the variables are correctly read
even for the last part, which is preceded by empty parts
in the variable files.

For example, this is the "scalar_per_node" variable file
(in text form for brevity; the actual file is in "C Binary" format):

    test scalar per node
    part
    1
    coordinates
    1.0 1.0 1.0
    part
    2
    part
    3
    coordinates
    part
    4
    coordinates
    4.0 4.0 4.0

"""

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtk.vtkGenericEnSightReader()
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/emptyParts_bin.case")
reader.ReadAllVariablesOn()
reader.Update()
case = reader.GetOutput()

assert case.GetNumberOfBlocks() == 4

part1 = case.GetBlock(0)
part2 = case.GetBlock(1)
part3 = case.GetBlock(2)
part4 = case.GetBlock(3)

assert part1.GetCellData().HasArray("scalar_per_element")
assert part1.GetCellData().HasArray("vector_per_element")
assert part1.GetPointData().HasArray("scalar_per_node")
assert part1.GetPointData().HasArray("vector_per_node")

assert part4.GetCellData().HasArray("scalar_per_element")
assert part4.GetCellData().HasArray("vector_per_element")
assert part4.GetPointData().HasArray("scalar_per_node")
assert part4.GetPointData().HasArray("vector_per_node")

# --- end of script --
