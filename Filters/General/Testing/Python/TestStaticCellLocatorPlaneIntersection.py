#!/usr/bin/env python
import vtk
from vtk.test import Testing

# Controls size of test
res = 15

# Create an initial set of data
wavelet = vtk.vtkRTAnalyticSource()
wavelet.SetWholeExtent(-res,res,-res,res,-res,res)
wavelet.Update()

# Isocontour
contour = vtk.vtkFlyingEdges3D()
contour.SetInputConnection(wavelet.GetOutputPort())
contour.SetValue(0,100)
contour.SetValue(1,200)
contour.Update()

# Build the locator
locator = vtk.vtkStaticCellLocator()
locator.SetDataSet(contour.GetOutput())
locator.AutomaticOn()
locator.SetNumberOfCellsPerNode(20)
locator.CacheCellBoundsOn()
locator.BuildLocator()

# Now extract cells from locator and display it
origin = [0,0,0]
normal = [1,1,1]
cellIds = vtk.vtkIdList()

timer = vtk.vtkTimerLog()
timer.StartTimer()
locator.FindCellsAlongPlane(origin,normal,0.0,cellIds);
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cell extraction: {0}".format(time))
print("Number cells extracted: {0}".format(cellIds.GetNumberOfIds()))

extract = vtk.vtkExtractCells()
extract.SetInputConnection(contour.GetOutputPort())
extract.SetCellList(cellIds);

mapper = vtk.vtkDataSetMapper()
#mapper.SetInputConnection(contour.GetOutputPort())
mapper.SetInputConnection(extract.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# In case we want to see the mesh and the cells that are extracted
meshMapper = vtk.vtkPolyDataMapper()
meshMapper.SetInputConnection(contour.GetOutputPort())
meshMapper.ScalarVisibilityOff()

meshActor = vtk.vtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
meshActor.GetProperty().SetColor(1,0,0)

# Outline around the entire dataset
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(wavelet.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)


# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.AddActor(outlineActor)
#ren1.AddActor(meshActor)

ren1.SetBackground(1,1,1)
renWin.SetSize(200,200)

iren.Initialize()
iren.Start()
