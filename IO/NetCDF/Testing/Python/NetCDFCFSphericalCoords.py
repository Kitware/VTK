#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This test checks netCDF reader.  It uses the CF convention.
# Open the file.
reader = vtk.vtkNetCDFCFReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/tos_O1_2001-2002.nc")
# Set the arrays we want to load.
reader.UpdateMetaData()
reader.SetVariableArrayStatus("tos",1)
reader.SetSphericalCoordinates(1)
aa = vtk.vtkAssignAttribute()
aa.SetInputConnection(reader.GetOutputPort())
aa.Assign("tos","SCALARS","CELL_DATA")
thresh = vtk.vtkThreshold()
thresh.SetInputConnection(aa.GetOutputPort())
thresh.ThresholdByLower(10000)
surface = vtk.vtkDataSetSurfaceFilter()
surface.SetInputConnection(thresh.GetOutputPort())
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.SetScalarRange(270,310)
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren = vtk.vtkRenderer()
ren.AddActor(actor)
renWin = vtk.vtkRenderWindow()
renWin.SetSize(200,200)
renWin.AddRenderer(ren)
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
