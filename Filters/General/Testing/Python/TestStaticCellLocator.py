#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    reference,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkGenericCell,
    vtkPolyData,
    vtkSphere,
    vtkStaticCellLocator,
)
from vtkmodules.vtkFiltersGeneral import vtkClipDataSet
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkImagingSources import vtkImageMandelbrotSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.test import Testing

# create a test dataset
#
math = vtkMath()

# Controls size of test
res = 15

# Create an initial set of points and associated dataset
mandel = vtkImageMandelbrotSource()
mandel.SetWholeExtent(-res,res,-res,res,-res,res)
mandel.Update()

sphere = vtkSphere()
sphere.SetCenter(mandel.GetOutput().GetCenter())
sphere.SetRadius(mandel.GetOutput().GetLength()/4)

# Clip data to spit out unstructured tets
clipper = vtkClipDataSet()
clipper.SetInputConnection(mandel.GetOutputPort())
clipper.SetClipFunction(sphere)
clipper.InsideOutOn()
clipper.Update()

output = clipper.GetOutput()
numCells = output.GetNumberOfCells()
bounds = output.GetBounds()
#print bounds

# Support subsequent method calls
genCell = vtkGenericCell()
t = reference(0.0)
x = [0,0,0]
pc = [0,0,0]
subId = reference(0)
cellId = reference(0)

# Build the locator
locator = vtkStaticCellLocator()
#locator = vtkCellLocator()
locator.SetDataSet(output)
locator.AutomaticOn()
locator.SetNumberOfCellsPerNode(20)
locator.CacheCellBoundsOn()
locator.BuildLocator()

# Now visualize the locator
locatorPD = vtkPolyData()
locator.GenerateRepresentation(0,locatorPD)

locatorMapper = vtkPolyDataMapper()
locatorMapper.SetInputData(locatorPD)

locatorActor = vtkActor()
locatorActor.SetMapper(locatorMapper)

# Outline around the entire dataset
outline = vtkOutlineFilter()
outline.SetInputConnection(mandel.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Intersect the clipped output with a ray
ray = vtkPolyData()
rayPts = vtkPoints()
rayPts.InsertPoint(0, -7.5,-5,-5)
rayPts.InsertPoint(1,  2.5, 2, 2.5)
rayLine = vtkCellArray()
rayLine.InsertNextCell(2)
rayLine.InsertCellPoint(0)
rayLine.InsertCellPoint(1)
ray.SetPoints(rayPts)
ray.SetLines(rayLine)

rayMapper = vtkPolyDataMapper()
rayMapper.SetInputData(ray)

rayActor = vtkActor()
rayActor.SetMapper(rayMapper)
rayActor.GetProperty().SetColor(0,1,0)

# Produce intersection point
hit = locator.IntersectWithLine(rayPts.GetPoint(0), rayPts.GetPoint(1), 0.001,
                                t, x, pc, subId, cellId, genCell)
#print ("Hit: {0}".format(hit))
#print ("CellId: {0}".format(cellId))
assert cellId == 209

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(locatorActor)
ren1.AddActor(rayActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(200,200)

iren.Initialize()
iren.Start()
