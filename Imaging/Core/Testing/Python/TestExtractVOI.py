#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# to mark the origin
sphere = vtk.vtkSphereSource()
sphere.SetRadius(2.0)

sphereMapper = vtk.vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereMapper.ImmediateModeRenderingOn()

sphereActor = vtk.vtkActor()
sphereActor.SetMapper(sphereMapper)

rt = vtk.vtkRTAnalyticSource()
rt.SetWholeExtent(-50, 50, -50, 50, 0, 0)

voi = vtk.vtkExtractVOI()
voi.SetInputConnection(rt.GetOutputPort())
voi.SetVOI(-11, 39, 5, 45, 0, 0)
voi.SetSampleRate(5, 5, 1)

# Get rid of ambiguous triagulation issues.
surf = vtk.vtkDataSetSurfaceFilter()
surf.SetInputConnection(voi.GetOutputPort())

tris = vtk.vtkTriangleFilter()
tris.SetInputConnection(surf.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(tris.GetOutputPort())
mapper.ImmediateModeRenderingOn()
mapper.SetScalarRange(130, 280)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren = vtk.vtkRenderer()
ren.AddActor(actor)
ren.AddActor(sphereActor)
ren.ResetCamera()

# camera = ren.GetActiveCamera()
# camera.SetPosition(68.1939, -23.4323, 12.6465)
# camera.SetViewUp(0.46563, 0.882375, 0.0678508)
# camera.SetFocalPoint(3.65707, 11.4552, 1.83509)
# camera.SetClippingRange(59.2626, 101.825)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

iren.Initialize()
#iren.Start()
