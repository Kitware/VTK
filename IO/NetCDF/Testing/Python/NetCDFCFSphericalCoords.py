#!/usr/bin/env python
from GetReader import get_reader
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
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

# This test checks netCDF reader.  It uses the CF convention.
# Open the file.
reader = get_reader(VTK_DATA_ROOT + "/Data/tos_O1_2001-2002.nc")
# Set the arrays we want to load.
reader.UpdateInformation()
info = reader.GetOutputInformation(0)
assert info.Has(vtkStreamingDemandDrivenPipeline.TIME_STEPS()), "Time dependent dataset does not have TIME_STEPS key"
# the exact times are not the same between NetCDFCF and XArray
# accessors, because XArray converts the original NetCDF datetime
# (based on a certain calendar) to numpy datetime64.
times = info.Get(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
assert len(times) == 24 and (times[23]) - times[0] == (705.0 - 15.0), "Number of time steps or time values are wrong."
reader.SetVariableArrayStatus("tos",1)
reader.SetSphericalCoordinates(1)
aa = vtkAssignAttribute()
aa.SetInputConnection(reader.GetOutputPort())
aa.Assign("tos","SCALARS","CELL_DATA")
thresh = vtkThreshold()
thresh.SetInputConnection(aa.GetOutputPort())
thresh.SetThresholdFunction(vtkThreshold.THRESHOLD_LOWER)
thresh.SetLowerThreshold(10000.0)
surface = vtkDataSetSurfaceFilter()
surface.SetInputConnection(thresh.GetOutputPort())
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(surface.GetOutputPort())
mapper.SetScalarRange(270,310)
actor = vtkActor()
actor.SetMapper(mapper)
ren = vtkRenderer()
ren.AddActor(actor)
renWin = vtkRenderWindow()
renWin.SetSize(200,200)
renWin.AddRenderer(ren)
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
