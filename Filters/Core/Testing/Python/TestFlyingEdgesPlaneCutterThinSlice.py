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
s.InsertTuple1(0,0.0)
s.InsertTuple1(1,1.0)
s.InsertTuple1(2,2.0)
s.InsertTuple1(3,3.0)
s.InsertTuple1(4,4.0)
s.InsertTuple1(5,5.0)
s.InsertTuple1(6,6.0)
s.InsertTuple1(7,7.0)

plane = vtk.vtkPlane()
plane.SetOrigin(0.5,0.5,0.5)
plane.SetNormal(1,1,1)

iso = vtk.vtkFlyingEdgesPlaneCutter()
iso.SetInputData(im)
iso.InterpolateAttributesOn()
iso.SetPlane(plane)
iso.Update()

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.SetScalarRange(iso.GetOutput().GetPointData().GetScalars().GetRange())

isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,0,0)
isoActor.GetProperty().SetOpacity(1)

outline = vtk.vtkOutlineFilter()
outline.SetInputData(im)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()

outlineActor.SetMapper(outlineMapper)

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
