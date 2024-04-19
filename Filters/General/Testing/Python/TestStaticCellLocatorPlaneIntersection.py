#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkIdList
from vtkmodules.vtkCommonDataModel import vtkStaticCellLocator
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkExtractCells,
    vtkFlyingEdges3D,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing

# Controls size of test
res = 15

# Create an initial set of data
wavelet = vtkRTAnalyticSource()
wavelet.SetWholeExtent(-res,res,-res,res,-res,res)
wavelet.Update()

# Isocontour
contour = vtkFlyingEdges3D()
contour.SetInputConnection(wavelet.GetOutputPort())
contour.SetValue(0,100)
contour.SetValue(1,200)
contour.Update()

# Build the locator
locator = vtkStaticCellLocator()
locator.SetDataSet(contour.GetOutput())
locator.AutomaticOn()
locator.SetNumberOfCellsPerNode(20)
locator.CacheCellBoundsOn()
locator.BuildLocator()

# Now extract cells from locator and display it
origin = [0,0,0]
normal = [1,1,1]
cellIds = vtkIdList()

timer = vtkTimerLog()
timer.StartTimer()
locator.FindCellsAlongPlane(origin,normal,0.0,cellIds);
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cell extraction: {0}".format(time))
print("Number cells extracted: {0}".format(cellIds.GetNumberOfIds()))

extract = vtkExtractCells()
extract.SetInputConnection(contour.GetOutputPort())
numberOfIds = cellIds.GetNumberOfIds()
firstHalf = int(numberOfIds / 2)
secondHalf = numberOfIds - firstHalf
cellIdsFirstHalf = vtkIdList()
for i in range(firstHalf):
    cellIdsFirstHalf.InsertNextId(cellIds.GetId(i))
cellIdsSecondHalf = vtkIdList()
for i in range(secondHalf):
    cellIdsSecondHalf.InsertNextId(cellIds.GetId(i + firstHalf))
extract.AddCellList(cellIdsFirstHalf)
extract.AddCellList(cellIdsSecondHalf)

mapper = vtkDataSetMapper()
#mapper.SetInputConnection(contour.GetOutputPort())
mapper.SetInputConnection(extract.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

# In case we want to see the mesh and the cells that are extracted
meshMapper = vtkPolyDataMapper()
meshMapper.SetInputConnection(contour.GetOutputPort())
meshMapper.ScalarVisibilityOff()

meshActor = vtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
meshActor.GetProperty().SetColor(1,0,0)

# Outline around the entire dataset
outline = vtkOutlineFilter()
outline.SetInputConnection(wavelet.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)


# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
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
