#!/usr/bin/env python

from vtkmodules.util.misc import vtkGetDataRoot
from vtkmodules.vtkIOXML import vtkXMLUnstructuredGridReader
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter

reader = vtkXMLUnstructuredGridReader()
reader.SetFileName(vtkGetDataRoot() + "/Data/convexPointSet.vtu")

geometryFilter = vtkGeometryFilter()
geometryFilter.SetInputConnection(reader.GetOutputPort())
geometryFilter.Update()

assert (geometryFilter.GetOutput().GetNumberOfPoints() == 84)
assert (geometryFilter.GetOutput().GetNumberOfCells() == 140)
