#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Control test size
res = 32

# Append three polydatas together and display
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(-1,-1,1)
plane.SetPoint1(1,-1,1)
plane.SetPoint2(-1,1,1)
plane.Update()

sphere = vtk.vtkSphereSource()
sphere.SetThetaResolution(2*res)
sphere.SetPhiResolution(res)
sphere.SetCenter(0,0,0)
sphere.SetRadius(0.75)
sphere.Update()

xform = vtk.vtkTransform()
xform.Translate(0,0,-2)

xformF = vtk.vtkTransformPolyDataFilter()
xformF.SetInputConnection(plane.GetOutputPort())
xformF.SetTransform(xform)

edges = vtk.vtkFeatureEdges()
edges.SetInputConnection(xformF.GetOutputPort())
edges.ExtractAllEdgeTypesOff()
edges.ManifoldEdgesOn()
edges.BoundaryEdgesOn()

append = vtk.vtkAppendPolyData()
append.AddInputConnection(plane.GetOutputPort())
append.AddInputConnection(edges.GetOutputPort())
append.AddInputConnection(sphere.GetOutputPort())
append.Update()

beforeMapper = vtk.vtkPolyDataMapper()
beforeMapper.SetInputConnection(append.GetOutputPort())

beforeActor = vtk.vtkActor()
beforeActor.SetMapper(beforeMapper)

# Subtract one and display
remove = vtk.vtkRemovePolyData()
remove.AddInputConnection(append.GetOutputPort())
remove.AddInputConnection(plane.GetOutputPort())
remove.Update()

# Subtract the first and second and display
append2 = vtk.vtkAppendPolyData()
append2.AddInputConnection(plane.GetOutputPort())
append2.AddInputConnection(edges.GetOutputPort())
append2.Update()

# We use an append filter because the side effect is to
# reorder the points so that they have the correct ids
# for subtracting from input(0).
remove2 = vtk.vtkRemovePolyData()
remove2.AddInputConnection(append.GetOutputPort())
remove2.AddInputConnection(append2.GetOutputPort())
remove2.Update()

afterMapper = vtk.vtkPolyDataMapper()
afterMapper.SetInputConnection(remove.GetOutputPort())

afterActor = vtk.vtkActor()
afterActor.SetMapper(afterMapper)

afterMapper2 = vtk.vtkPolyDataMapper()
afterMapper2.SetInputConnection(remove2.GetOutputPort())

afterActor2 = vtk.vtkActor()
afterActor2.SetMapper(afterMapper2)

# Finally, test removing all cells
remove3 = vtk.vtkRemovePolyData()
remove3.AddInputConnection(append.GetOutputPort())
remove3.AddInputConnection(append.GetOutputPort())
remove3.Update()

afterMapper3 = vtk.vtkPolyDataMapper()
afterMapper3.SetInputConnection(remove3.GetOutputPort())

afterActor3 = vtk.vtkActor()
afterActor3.SetMapper(afterMapper3)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.25,1.0)
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.25,0,0.5,1.0)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0.5,0,0.75,1.0)
ren4 = vtk.vtkRenderer()
ren4.SetViewport(0.75,0,1.0,1.0)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(beforeActor)
ren2.AddActor(afterActor)
ren3.AddActor(afterActor2)
ren4.AddActor(afterActor3)

renWin.SetSize(600,200)
ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)
ren3.SetBackground(0.1, 0.2, 0.4)
ren4.SetBackground(0.1, 0.2, 0.4)
ren2.SetActiveCamera(ren1.GetActiveCamera())
ren3.SetActiveCamera(ren1.GetActiveCamera())
ren4.SetActiveCamera(ren1.GetActiveCamera())

ren1.GetActiveCamera().SetPosition(1,0,0.5)
ren1.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()
iren.Start()
