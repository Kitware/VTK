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

# Create a synthetic source: a volume with a single voxel
im = vtk.vtkImageData()
im.SetDimensions(2,2,2)
im.AllocateScalars(10,1)
s = im.GetPointData().GetScalars()
s.InsertTuple1(0,-1.0)
s.InsertTuple1(1,-1.0)
s.InsertTuple1(2,-1.0)
s.InsertTuple1(3,-1.0)
s.InsertTuple1(4,1.0)
s.InsertTuple1(5,1.0)
s.InsertTuple1(6,1.0)
s.InsertTuple1(7,1.0)

iso = vtk.vtkFlyingEdges3D()
iso.SetInputData(im)
iso.SetValue(0,0.0)
iso.ComputeNormalsOff()
iso.ComputeGradientsOff()
iso.ComputeScalarsOn()
iso.InterpolateAttributesOff()
iso.Update()

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,1,1)
isoActor.GetProperty().SetOpacity(1)

outline = vtk.vtkOutlineFilter()
outline.SetInputData(im)

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
iren.Start()
# --- end of script --
