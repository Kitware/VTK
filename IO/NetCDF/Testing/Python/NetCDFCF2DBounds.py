#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF CF reader for reading 2D bounds information.
renWin = vtk.vtkRenderWindow()
renWin.SetSize(400,200)
#############################################################################
# Case 1: Spherical coordinates off.
# Open the file.
reader_cartesian = vtk.vtkNetCDFCFReader()
reader_cartesian.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sampleCurveGrid4.nc")
# Set the arrays we want to load.
reader_cartesian.UpdateMetaData()
reader_cartesian.SetVariableArrayStatus("sample",1)
reader_cartesian.SphericalCoordinatesOff()
# Extract a region that is non-overlapping.
voi_cartesian = vtk.vtkExtractGrid()
voi_cartesian.SetInputConnection(reader_cartesian.GetOutputPort())
voi_cartesian.SetVOI(20,47,0,31,0,0)
# Assign the field to scalars.
aa_cartesian = vtk.vtkAssignAttribute()
aa_cartesian.SetInputConnection(voi_cartesian.GetOutputPort())
aa_cartesian.Assign("sample","SCALARS","CELL_DATA")
# Extract a surface that we can render.
surface_cartesian = vtk.vtkDataSetSurfaceFilter()
surface_cartesian.SetInputConnection(aa_cartesian.GetOutputPort())
mapper_cartesian = vtk.vtkPolyDataMapper()
mapper_cartesian.SetInputConnection(surface_cartesian.GetOutputPort())
mapper_cartesian.SetScalarRange(0,1535)
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
reader_spherical.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sampleCurveGrid4.nc")
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
mapper_spherical.SetScalarRange(0,1535)
actor_spherical = vtk.vtkActor()
actor_spherical.SetMapper(mapper_spherical)
ren_spherical = vtk.vtkRenderer()
ren_spherical.AddActor(actor_spherical)
ren_spherical.SetViewport(0.5,0.0,1.0,1.0)
renWin.AddRenderer(ren_spherical)
# --- end of script --
