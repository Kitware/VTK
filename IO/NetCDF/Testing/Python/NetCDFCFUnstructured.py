#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF CF reader for reading unstructured (p-sided) cells.
renWin = vtk.vtkRenderWindow()
renWin.SetSize(400,200)
#############################################################################
# Case 1: Spherical coordinates off.
# Open the file.
reader_cartesian = vtk.vtkNetCDFCFReader()
reader_cartesian.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sampleGenGrid3.nc")
# Set the arrays we want to load.
reader_cartesian.UpdateMetaData()
reader_cartesian.SetVariableArrayStatus("sample",1)
reader_cartesian.SphericalCoordinatesOff()
# Assign the field to scalars.
aa_cartesian = vtk.vtkAssignAttribute()
aa_cartesian.SetInputConnection(reader_cartesian.GetOutputPort())
aa_cartesian.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_cartesian = vtk.vtkDataSetSurfaceFilter()
surface_cartesian.SetInputConnection(aa_cartesian.GetOutputPort())
mapper_cartesian = vtk.vtkPolyDataMapper()
mapper_cartesian.SetInputConnection(surface_cartesian.GetOutputPort())
mapper_cartesian.SetScalarRange(100,2500)
actor_cartesian = vtk.vtkActor()
actor_cartesian.SetMapper(mapper_cartesian)
ren_cartesian = vtk.vtkRenderer()
ren_cartesian.AddActor(actor_cartesian)
ren_cartesian.SetViewport(0.0,0.0,0.5,1.0)
renWin.AddRenderer(ren_cartesian)
#############################################################################
# Case 2: Spherical coordinates on.
# Open the file.
reader_spherical = vtk.vtkNetCDFCFReader()
reader_spherical.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sampleGenGrid3.nc")
# Set the arrays we want to load.
reader_spherical.UpdateMetaData()
reader_spherical.SetVariableArrayStatus("sample",1)
reader_spherical.SphericalCoordinatesOn()
# Assign the field to scalars.
aa_spherical = vtk.vtkAssignAttribute()
aa_spherical.SetInputConnection(reader_spherical.GetOutputPort())
aa_spherical.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_spherical = vtk.vtkDataSetSurfaceFilter()
surface_spherical.SetInputConnection(aa_spherical.GetOutputPort())
mapper_spherical = vtk.vtkPolyDataMapper()
mapper_spherical.SetInputConnection(surface_spherical.GetOutputPort())
mapper_spherical.SetScalarRange(100,2500)
actor_spherical = vtk.vtkActor()
actor_spherical.SetMapper(mapper_spherical)
ren_spherical = vtk.vtkRenderer()
ren_spherical.AddActor(actor_spherical)
ren_spherical.SetViewport(0.5,0.0,1.0,1.0)
renWin.AddRenderer(ren_spherical)
#############################################################################
# Case 3: Read as structured data.
# The resulting data is garbage, so we won't actually look at the output.
# This is just to verify that nothing errors out or crashes.
reader_structured = vtk.vtkNetCDFCFReader()
reader_structured.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sampleGenGrid3.nc")
reader_structured.UpdateMetaData()
reader_structured.SetVariableArrayStatus("sample",1)
reader_structured.SphericalCoordinatesOn()
reader_structured.SetOutputTypeToImage()
reader_structured.Update()
reader_structured.SetOutputTypeToRectilinear()
reader_structured.Update()
reader_structured.SetOutputTypeToStructured()
reader_structured.Update()
# --- end of script --
