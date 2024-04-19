#!/usr/bin/env python

"""
vtkEnSightWriter test for writing node IDs and element IDs

This test exercises the WriteNodeIDs and WriteElementIDs
options of vtkEnSightWriter. Since vtkEnSightGoldBinaryReader
currently provides no way to retrieve node/element IDs, we just
test that the files are readable and contain geometry.

"""

import vtk
from vtk.util.misc import vtkGetTempDir
import os.path
VTK_TEMP_DIR = vtkGetTempDir()

cell_type_source = vtk.vtkCellTypeSource()
cell_type_source.SetCellType(vtk.VTK_HEXAHEDRON)
cell_type_source.Update()
reference_mesh = cell_type_source.GetOutput()

for write_node_ids in range(2):
    for write_element_ids in range(2):
        filename = "ensightWriterNodeIDs{}ElementIDs{}".format(write_node_ids, write_element_ids)

        writer = vtk.vtkEnSightWriter()
        writer.SetFileName(os.path.join(VTK_TEMP_DIR, "{}.case".format(filename)))
        writer.SetInputData(reference_mesh)
        writer.SetWriteNodeIDs(write_node_ids)
        writer.SetWriteElementIDs(write_element_ids)
        writer.Write()
        writer.WriteCaseFile(0)

        reader = vtk.vtkEnSightGoldBinaryReader()
        reader.SetCaseFileName(os.path.join(VTK_TEMP_DIR, "{}.0.case".format(filename)))
        reader.Update()
        mesh = reader.GetOutput().GetBlock(0)

        assert mesh.GetNumberOfPoints() == reference_mesh.GetNumberOfPoints()
        assert mesh.GetNumberOfCells() == reference_mesh.GetNumberOfCells()
