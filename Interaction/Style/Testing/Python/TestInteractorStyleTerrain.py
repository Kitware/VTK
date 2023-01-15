#!/usr/bin/env python

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import (
    vtkElevationFilter,
    vtkPolyDataNormals,
)
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkIOImage import vtkDEMReader
from vtkmodules.vtkImagingCore import vtkImageShrink3D
from vtkmodules.vtkInteractionStyle import vtkInteractorStyleTerrain
from vtkmodules.vtkRenderingCore import (
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

Scale = 5
lut = vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

demModel = vtkDEMReader()
demModel.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demModel.Update()

print(demModel)

lo = Scale * demModel.GetElevationBounds()[0]
hi = Scale * demModel.GetElevationBounds()[1]

demActor = vtkLODActor()

# create a pipeline for each lod mapper
shrink16 = vtkImageShrink3D()
shrink16.SetShrinkFactors(16, 16, 1)
shrink16.SetInputConnection(demModel.GetOutputPort())
shrink16.AveragingOn()

geom16 = vtkImageDataGeometryFilter()
geom16.SetInputConnection(shrink16.GetOutputPort())
geom16.ReleaseDataFlagOn()

warp16 = vtkWarpScalar()
warp16.SetInputConnection(geom16.GetOutputPort())
warp16.SetNormal(0, 0, 1)
warp16.UseNormalOn()
warp16.SetScaleFactor(Scale)
warp16.ReleaseDataFlagOn()

elevation16 = vtkElevationFilter()
elevation16.SetInputConnection(warp16.GetOutputPort())
elevation16.SetLowPoint(0, 0, lo)
elevation16.SetHighPoint(0, 0, hi)
elevation16.SetScalarRange(lo, hi)
elevation16.ReleaseDataFlagOn()

normals16 = vtkPolyDataNormals()
normals16.SetInputConnection(elevation16.GetOutputPort())
normals16.SetFeatureAngle(60)
normals16.ConsistencyOff()
normals16.SplittingOff()
normals16.ReleaseDataFlagOn()

demMapper16 = vtkPolyDataMapper()
demMapper16.SetInputConnection(normals16.GetOutputPort())
demMapper16.SetScalarRange(lo, hi)
demMapper16.SetLookupTable(lut)

demMapper16.Update()
demActor.AddLODMapper(demMapper16)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
t = vtkInteractorStyleTerrain()
iren.SetInteractorStyle(t)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(demActor)
ren.SetBackground(.4, .4, .4)

iren.SetDesiredUpdateRate(1)
def TkCheckAbort(obj=None, event=""):
    if renWin.GetEventPending():
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)
ren.GetActiveCamera().SetViewUp(0, 0, 1)
ren.GetActiveCamera().SetPosition(-99900, -21354, 131801)
ren.GetActiveCamera().SetFocalPoint(41461, 41461, 2815)
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()
renWin.Render()
