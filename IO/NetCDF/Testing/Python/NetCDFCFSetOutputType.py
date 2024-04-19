#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkAssignAttribute,
    vtkThreshold,
)
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkIONetCDF import vtkNetCDFCFReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF reader.  It uses the COARDS convention.
renWin = vtkRenderWindow()
renWin.SetSize(400,400)
#############################################################################
# Case 1: Image type.
# Open the file.
reader_image = vtkNetCDFCFReader()
reader_image.SetFileName(VTK_DATA_ROOT + "/Data/tos_O1_2001-2002.nc")
reader_image.SetOutputTypeToImage()
# Set the arrays we want to load.
reader_image.UpdateMetaData()
reader_image.SetVariableArrayStatus("tos",1)
reader_image.SphericalCoordinatesOff()
aa_image = vtkAssignAttribute()
aa_image.SetInputConnection(reader_image.GetOutputPort())
aa_image.Assign("tos","SCALARS","POINT_DATA")
thresh_image = vtkThreshold()
thresh_image.SetInputConnection(aa_image.GetOutputPort())
thresh_image.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
thresh_image.SetLowerThreshold(10000.0)
surface_image = vtkDataSetSurfaceFilter()
surface_image.SetInputConnection(thresh_image.GetOutputPort())
mapper_image = vtkPolyDataMapper()
mapper_image.SetInputConnection(surface_image.GetOutputPort())
mapper_image.SetScalarRange(270,310)
actor_image = vtkActor()
actor_image.SetMapper(mapper_image)
ren_image = vtkRenderer()
ren_image.AddActor(actor_image)
ren_image.SetViewport(0.0,0.0,0.5,0.5)
renWin.AddRenderer(ren_image)
#############################################################################
# Case 2: Rectilinear type.
# Open the file.
reader_rect = vtkNetCDFCFReader()
reader_rect.SetFileName(VTK_DATA_ROOT + "/Data/tos_O1_2001-2002.nc")
reader_rect.SetOutputTypeToRectilinear()
# Set the arrays we want to load.
reader_rect.UpdateMetaData()
reader_rect.SetVariableArrayStatus("tos",1)
reader_rect.SphericalCoordinatesOff()
aa_rect = vtkAssignAttribute()
aa_rect.SetInputConnection(reader_rect.GetOutputPort())
aa_rect.Assign("tos","SCALARS","POINT_DATA")
thresh_rect = vtkThreshold()
thresh_rect.SetInputConnection(aa_rect.GetOutputPort())
thresh_rect.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
thresh_rect.SetLowerThreshold(10000.0)
surface_rect = vtkDataSetSurfaceFilter()
surface_rect.SetInputConnection(thresh_rect.GetOutputPort())
mapper_rect = vtkPolyDataMapper()
mapper_rect.SetInputConnection(surface_rect.GetOutputPort())
mapper_rect.SetScalarRange(270,310)
actor_rect = vtkActor()
actor_rect.SetMapper(mapper_rect)
ren_rect = vtkRenderer()
ren_rect.AddActor(actor_rect)
ren_rect.SetViewport(0.5,0.0,1.0,0.5)
renWin.AddRenderer(ren_rect)
#############################################################################
# Case 3: Structured type.
# Open the file.
reader_struct = vtkNetCDFCFReader()
reader_struct.SetFileName(VTK_DATA_ROOT + "/Data/tos_O1_2001-2002.nc")
reader_struct.SetOutputTypeToStructured()
# Set the arrays we want to load.
reader_struct.UpdateMetaData()
reader_struct.SetVariableArrayStatus("tos",1)
reader_struct.SphericalCoordinatesOff()
aa_struct = vtkAssignAttribute()
aa_struct.SetInputConnection(reader_struct.GetOutputPort())
aa_struct.Assign("tos","SCALARS","POINT_DATA")
thresh_struct = vtkThreshold()
thresh_struct.SetInputConnection(aa_struct.GetOutputPort())
thresh_struct.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
thresh_struct.SetLowerThreshold(10000.0)
surface_struct = vtkDataSetSurfaceFilter()
surface_struct.SetInputConnection(thresh_struct.GetOutputPort())
mapper_struct = vtkPolyDataMapper()
mapper_struct.SetInputConnection(surface_struct.GetOutputPort())
mapper_struct.SetScalarRange(270,310)
actor_struct = vtkActor()
actor_struct.SetMapper(mapper_struct)
ren_struct = vtkRenderer()
ren_struct.AddActor(actor_struct)
ren_struct.SetViewport(0.0,0.5,0.5,1.0)
renWin.AddRenderer(ren_struct)
#############################################################################
# Case 4: Unstructured type.
# Open the file.
reader_auto = vtkNetCDFCFReader()
reader_auto.SetFileName(VTK_DATA_ROOT + "/Data/tos_O1_2001-2002.nc")
reader_auto.SetOutputTypeToUnstructured()
# Set the arrays we want to load.
reader_auto.UpdateMetaData()
reader_auto.SetVariableArrayStatus("tos",1)
reader_auto.SphericalCoordinatesOff()
aa_auto = vtkAssignAttribute()
aa_auto.SetInputConnection(reader_auto.GetOutputPort())
aa_auto.Assign("tos","SCALARS","POINT_DATA")
thresh_auto = vtkThreshold()
thresh_auto.SetInputConnection(aa_auto.GetOutputPort())
thresh_auto.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
thresh_auto.SetLowerThreshold(10000.0)
surface_auto = vtkDataSetSurfaceFilter()
surface_auto.SetInputConnection(thresh_auto.GetOutputPort())
mapper_auto = vtkPolyDataMapper()
mapper_auto.SetInputConnection(surface_auto.GetOutputPort())
mapper_auto.SetScalarRange(270,310)
actor_auto = vtkActor()
actor_auto.SetMapper(mapper_auto)
ren_auto = vtkRenderer()
ren_auto.AddActor(actor_auto)
ren_auto.SetViewport(0.5,0.5,1.0,1.0)
renWin.AddRenderer(ren_auto)
iren = vtkRenderWindowInteractor()
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
