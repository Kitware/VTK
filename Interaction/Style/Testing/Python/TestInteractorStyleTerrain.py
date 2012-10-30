#!/usr/bin/env python

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

Scale = 5
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

demModel = vtk.vtkDEMReader()
demModel.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demModel.Update()

print demModel

lo = Scale * demModel.GetElevationBounds()[0]
hi = Scale * demModel.GetElevationBounds()[1]

demActor = vtk.vtkLODActor()

# create a pipeline for each lod mapper
shrink16 = vtk.vtkImageShrink3D()
shrink16.SetShrinkFactors(16, 16, 1)
shrink16.SetInputConnection(demModel.GetOutputPort())
shrink16.AveragingOn()

geom16 = vtk.vtkImageDataGeometryFilter()
geom16.SetInputConnection(shrink16.GetOutputPort())
geom16.ReleaseDataFlagOn()

warp16 = vtk.vtkWarpScalar()
warp16.SetInputConnection(geom16.GetOutputPort())
warp16.SetNormal(0, 0, 1)
warp16.UseNormalOn()
warp16.SetScaleFactor(Scale)
warp16.ReleaseDataFlagOn()

elevation16 = vtk.vtkElevationFilter()
elevation16.SetInputConnection(warp16.GetOutputPort())
elevation16.SetLowPoint(0, 0, lo)
elevation16.SetHighPoint(0, 0, hi)
elevation16.SetScalarRange(lo, hi)
elevation16.ReleaseDataFlagOn()

normals16 = vtk.vtkPolyDataNormals()
normals16.SetInputConnection(elevation16.GetOutputPort())
normals16.SetFeatureAngle(60)
normals16.ConsistencyOff()
normals16.SplittingOff()
normals16.ReleaseDataFlagOn()

demMapper16 = vtk.vtkPolyDataMapper()
demMapper16.SetInputConnection(normals16.GetOutputPort())
demMapper16.SetScalarRange(lo, hi)
demMapper16.SetLookupTable(lut)
demMapper16.ImmediateModeRenderingOn()

demMapper16.Update()
demActor.AddLODMapper(demMapper16)

# Create the RenderWindow, Renderer and both Actors
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
t = vtk.vtkInteractorStyleTerrain()
iren.SetInteractorStyle(t)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(demActor)
ren.SetBackground(.4, .4, .4)

iren.SetDesiredUpdateRate(1)
def TkCheckAbort (__vtk__temp0=0, __vtk__temp1=0):
    foo = renWin.GetEventPending()
    if (foo != 0):
        renWin.SetAbortRender(1)
        pass

renWin.AddObserver("AbortCheckEvent", TkCheckAbort)
ren.GetActiveCamera().SetViewUp(0, 0, 1)
ren.GetActiveCamera().SetPosition(-99900, -21354, 131801)
ren.GetActiveCamera().SetFocalPoint(41461, 41461, 2815)
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()
renWin.Render()
