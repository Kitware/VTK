#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer
#
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren )
renWin.SetSize(600,200)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Read input dataset that has n-faced polyhedra
reader = vtk.vtkExodusIIReader()
reader.SetFileName(str(VTK_DATA_ROOT) + "/Data/cube-1.exo")
reader.Update()
dataset = reader.GetOutput()

# clip the dataset
clipper = vtk.vtkClipDataSet()
clipper.SetInputData(dataset.GetBlock(0).GetBlock(0))
plane = vtk.vtkPlane()
plane.SetNormal(0.5,0.5,0.5)
plane.SetOrigin(0.5,0.5,0.5)
clipper.SetClipFunction(plane)
clipper.Update()

# get surface representation to render
surfaceFilter = vtk.vtkDataSetSurfaceFilter()
surfaceFilter.SetInputData(clipper.GetOutput())
surfaceFilter.Update()
surface = surfaceFilter.GetOutput()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputData(surfaceFilter.GetOutput())
mapper.Update()

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToSurface()
actor.GetProperty().EdgeVisibilityOn()

ren.AddActor(actor)

ren.GetActiveCamera().SetPosition(-0.5,0.5,0)
ren.GetActiveCamera().SetFocalPoint(0.5, 0.5, 0.5)
ren.GetActiveCamera().SetViewUp(0.0820, 0.934, -0.348)
ren.ResetCamera()
renWin.Render()
iren.Start()
