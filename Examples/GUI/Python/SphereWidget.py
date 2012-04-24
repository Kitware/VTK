#!/usr/bin/env python

# This example demonstrates how to use the vtkSphereWidget to control
# the position of a light.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Start by loading some data.
dem = vtk.vtkDEMReader()
dem.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
dem.Update()

Scale = 2
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.6, 0)
lut.SetSaturationRange(1.0, 0)
lut.SetValueRange(0.5, 1.0)

lo = Scale*dem.GetElevationBounds()[0]
hi = Scale*dem.GetElevationBounds()[1]

shrink = vtk.vtkImageShrink3D()
shrink.SetShrinkFactors(4, 4, 1)
shrink.SetInputConnection(dem.GetOutputPort())
shrink.AveragingOn()

geom = vtk.vtkImageDataGeometryFilter()
geom.SetInputConnection(shrink.GetOutputPort())
geom.ReleaseDataFlagOn()

warp = vtk.vtkWarpScalar()
warp.SetInputConnection(geom.GetOutputPort())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(Scale)
warp.ReleaseDataFlagOn()

elevation = vtk.vtkElevationFilter()
elevation.SetInputConnection(warp.GetOutputPort())
elevation.SetLowPoint(0, 0, lo)
elevation.SetHighPoint(0, 0, hi)
elevation.SetScalarRange(lo, hi)
elevation.ReleaseDataFlagOn()

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(elevation.GetOutputPort())
normals.SetFeatureAngle(60)
normals.ConsistencyOff()
normals.SplittingOff()
normals.ReleaseDataFlagOn()

demMapper = vtk.vtkPolyDataMapper()
demMapper.SetInputConnection(normals.GetOutputPort())
demMapper.SetScalarRange(lo, hi)
demMapper.SetLookupTable(lut)
demMapper.ImmediateModeRenderingOn()

demActor = vtk.vtkLODActor()
demActor.SetMapper(demMapper)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.LightFollowCameraOff()
iren.SetInteractorStyle(None)

# Associate the line widget with the interactor
sphereWidget = vtk.vtkSphereWidget()
sphereWidget.SetInteractor(iren)
sphereWidget.SetProp3D(demActor)
sphereWidget.SetPlaceFactor(4)
sphereWidget.PlaceWidget()
sphereWidget.TranslationOff()
sphereWidget.ScaleOff()
sphereWidget.HandleVisibilityOn()

# Uncomment the next line if you want to see the widget active when
# the script starts
#sphereWidget.EnabledOn()

# Actually probe the data
def MoveLight(obj, event):
    global light
    light.SetPosition(obj.GetHandlePosition())

sphereWidget.AddObserver("InteractionEvent", MoveLight)

# Add the actors to the renderer, set the background and size
ren.AddActor(demActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetViewUp(0, 0, 1)
cam1.SetFocalPoint(dem.GetOutput().GetCenter())
cam1.SetPosition(1, 0, 0)
ren.ResetCamera()
cam1.Elevation(25)
cam1.Azimuth(125)
cam1.Zoom(1.25)

light = vtk.vtkLight()
light.SetFocalPoint(dem.GetOutput().GetCenter())
ren.AddLight(light)

iren.Initialize()
renWin.Render()
iren.Start()
