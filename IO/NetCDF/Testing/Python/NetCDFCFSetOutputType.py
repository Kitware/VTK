#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF reader.  It uses the COARDS convention.
renWin = vtk.vtkRenderWindow()
renWin.SetSize(400,400)
#############################################################################
# Case 1: Image type.
# Open the file.
reader_image = vtk.vtkNetCDFCFReader()
reader_image.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_image.SetOutputTypeToImage()
# Set the arrays we want to load.
reader_image.UpdateMetaData()
reader_image.SetVariableArrayStatus("tos",1)
reader_image.SphericalCoordinatesOff()
aa_image = vtk.vtkAssignAttribute()
aa_image.SetInputConnection(reader_image.GetOutputPort())
aa_image.Assign("tos","SCALARS","POINT_DATA")
thresh_image = vtk.vtkThreshold()
thresh_image.SetInputConnection(aa_image.GetOutputPort())
thresh_image.ThresholdByLower(10000)
surface_image = vtk.vtkDataSetSurfaceFilter()
surface_image.SetInputConnection(thresh_image.GetOutputPort())
mapper_image = vtk.vtkPolyDataMapper()
mapper_image.SetInputConnection(surface_image.GetOutputPort())
mapper_image.SetScalarRange(270,310)
actor_image = vtk.vtkActor()
actor_image.SetMapper(mapper_image)
ren_image = vtk.vtkRenderer()
ren_image.AddActor(actor_image)
ren_image.SetViewport(0.0,0.0,0.5,0.5)
renWin.AddRenderer(ren_image)
#############################################################################
# Case 2: Rectilinear type.
# Open the file.
reader_rect = vtk.vtkNetCDFCFReader()
reader_rect.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_rect.SetOutputTypeToRectilinear()
# Set the arrays we want to load.
reader_rect.UpdateMetaData()
reader_rect.SetVariableArrayStatus("tos",1)
reader_rect.SphericalCoordinatesOff()
aa_rect = vtk.vtkAssignAttribute()
aa_rect.SetInputConnection(reader_rect.GetOutputPort())
aa_rect.Assign("tos","SCALARS","POINT_DATA")
thresh_rect = vtk.vtkThreshold()
thresh_rect.SetInputConnection(aa_rect.GetOutputPort())
thresh_rect.ThresholdByLower(10000)
surface_rect = vtk.vtkDataSetSurfaceFilter()
surface_rect.SetInputConnection(thresh_rect.GetOutputPort())
mapper_rect = vtk.vtkPolyDataMapper()
mapper_rect.SetInputConnection(surface_rect.GetOutputPort())
mapper_rect.SetScalarRange(270,310)
actor_rect = vtk.vtkActor()
actor_rect.SetMapper(mapper_rect)
ren_rect = vtk.vtkRenderer()
ren_rect.AddActor(actor_rect)
ren_rect.SetViewport(0.5,0.0,1.0,0.5)
renWin.AddRenderer(ren_rect)
#############################################################################
# Case 3: Structured type.
# Open the file.
reader_struct = vtk.vtkNetCDFCFReader()
reader_struct.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_struct.SetOutputTypeToStructured()
# Set the arrays we want to load.
reader_struct.UpdateMetaData()
reader_struct.SetVariableArrayStatus("tos",1)
reader_struct.SphericalCoordinatesOff()
aa_struct = vtk.vtkAssignAttribute()
aa_struct.SetInputConnection(reader_struct.GetOutputPort())
aa_struct.Assign("tos","SCALARS","POINT_DATA")
thresh_struct = vtk.vtkThreshold()
thresh_struct.SetInputConnection(aa_struct.GetOutputPort())
thresh_struct.ThresholdByLower(10000)
surface_struct = vtk.vtkDataSetSurfaceFilter()
surface_struct.SetInputConnection(thresh_struct.GetOutputPort())
mapper_struct = vtk.vtkPolyDataMapper()
mapper_struct.SetInputConnection(surface_struct.GetOutputPort())
mapper_struct.SetScalarRange(270,310)
actor_struct = vtk.vtkActor()
actor_struct.SetMapper(mapper_struct)
ren_struct = vtk.vtkRenderer()
ren_struct.AddActor(actor_struct)
ren_struct.SetViewport(0.0,0.5,0.5,1.0)
renWin.AddRenderer(ren_struct)
#############################################################################
# Case 4: Unstructured type.
# Open the file.
reader_auto = vtk.vtkNetCDFCFReader()
reader_auto.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
reader_auto.SetOutputTypeToUnstructured()
# Set the arrays we want to load.
reader_auto.UpdateMetaData()
reader_auto.SetVariableArrayStatus("tos",1)
reader_auto.SphericalCoordinatesOff()
aa_auto = vtk.vtkAssignAttribute()
aa_auto.SetInputConnection(reader_auto.GetOutputPort())
aa_auto.Assign("tos","SCALARS","POINT_DATA")
thresh_auto = vtk.vtkThreshold()
thresh_auto.SetInputConnection(aa_auto.GetOutputPort())
thresh_auto.ThresholdByLower(10000)
surface_auto = vtk.vtkDataSetSurfaceFilter()
surface_auto.SetInputConnection(thresh_auto.GetOutputPort())
mapper_auto = vtk.vtkPolyDataMapper()
mapper_auto.SetInputConnection(surface_auto.GetOutputPort())
mapper_auto.SetScalarRange(270,310)
actor_auto = vtk.vtkActor()
actor_auto.SetMapper(mapper_auto)
ren_auto = vtk.vtkRenderer()
ren_auto.AddActor(actor_auto)
ren_auto.SetViewport(0.5,0.5,1.0,1.0)
renWin.AddRenderer(ren_auto)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
# # Setup a lookup table.
# vtkLookupTable lut
# lut SetTableRange 270 310
# lut SetHueRange 0.66 0.0
# lut SetRampToLinear
# # Make pretty colors
# vtkImageMapToColors map
# map SetInputConnection [asinine GetOutputPort]
# map SetLookupTable lut
# map SetOutputFormatToRGB
# # vtkImageViewer viewer
# # viewer SetInputConnection [map GetOutputPort]
# # viewer SetColorWindow 256
# # viewer SetColorLevel 127.5
# # viewer Render
# vtkImageViewer2 viewer
# viewer SetInputConnection [map GetOutputPort]
# viewer Render
# --- end of script --
