#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

res = 100
# Test the volume constraint. Create "planar" data with jittery
# z-direction coordinates. Decimate the data with and without
# planar constraints and compare.
#
ps = vtk.vtkPlaneSource()
ps.SetResolution(res,res)

tf = vtk.vtkTriangleFilter()
tf.SetInputConnection(ps.GetOutputPort())

attr = vtk.vtkRandomAttributeGenerator()
attr.SetInputConnection(tf.GetOutputPort())
attr.GeneratePointScalarsOn()
attr.SetMinimumComponentValue(-1)
attr.SetMaximumComponentValue(1)

# This jitters the geometry
warp = vtk.vtkWarpScalar()
warp.SetInputConnection(attr.GetOutputPort())
warp.SetScaleFactor(0.02)

# Decimator without volume constraint
deci = vtk.vtkQuadricDecimation()
deci.SetInputConnection(warp.GetOutputPort())
deci.SetTargetReduction(.95)
deci.AttributeErrorMetricOn()
deci.VolumePreservationOff()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(deci.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Decimator with volume constraint
deci2 = vtk.vtkQuadricDecimation()
deci2.SetInputConnection(warp.GetOutputPort())
deci2.SetTargetReduction(.95)
deci2.AttributeErrorMetricOn()
deci2.VolumePreservationOn()

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(deci2.GetOutputPort())

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)

# Create rendering instances
#
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,.5,1)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Set up the camera parameters
#
camera = vtk.vtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(0,0,1)
ren0.SetActiveCamera(camera)
ren1.SetActiveCamera(camera)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(actor)
ren1.AddActor(actor2)
ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
renWin.SetSize(600,300)
ren0.ResetCamera()
renWin.Render()
iren.Initialize()

# gather some information
print ( "Bounds (volume preserve off): ({0})".format( deci.GetOutput().GetPoints().GetBounds() ) )
print ( "Bounds (volume preserve on): ({0})".format( deci2.GetOutput().GetPoints().GetBounds() ) )

#iren.Start()
# --- end of script --
