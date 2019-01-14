#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source
sphere = vtk.vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

# Iso-surface to create geometry. This uses a very small volume to stress
# boundary conditions in vtkFlyingEdges; i.e., we want the sphere isosurface
# to intersect the boundary.
sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(3,3,3)

iso = vtk.vtkFlyingEdges3D()
iso.SetInputConnection(sample.GetOutputPort())
iso.SetValue(0,0.25)
iso.ComputeNormalsOn()
iso.ComputeGradientsOn()
iso.ComputeScalarsOn()
iso.InterpolateAttributesOff()

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(1)

outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(400,400)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
# --- end of script --
