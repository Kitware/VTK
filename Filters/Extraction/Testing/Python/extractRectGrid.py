#!/usr/bin/env python
# import os
from vtkmodules.vtkFiltersCore import vtkTriangleFilter
from vtkmodules.vtkFiltersExtraction import vtkExtractRectilinearGrid
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOLegacy import vtkRectilinearGridReader
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
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create pipeline - rectilinear grid
#
rgridReader = vtkRectilinearGridReader()
rgridReader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtk")

outline = vtkOutlineFilter()
outline.SetInputConnection(rgridReader.GetOutputPort())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(outline.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)

rgridReader.Update()

extract1 = vtkExtractRectilinearGrid()
extract1.SetInputConnection(rgridReader.GetOutputPort())
# extract1.SetVOI(0, 46, 0, 32, 0, 10)
extract1.SetVOI(23, 40, 16, 30, 9, 9)
extract1.SetSampleRate(2, 2, 1)
extract1.IncludeBoundaryOn()
extract1.Update()

surf1 = vtkDataSetSurfaceFilter()
surf1.SetInputConnection(extract1.GetOutputPort())

tris = vtkTriangleFilter()
tris.SetInputConnection(surf1.GetOutputPort())

mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(tris.GetOutputPort())
mapper1.SetScalarRange(extract1.GetOutput().GetScalarRange())

actor1 = vtkActor()
actor1.SetMapper(mapper1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# ren1.AddActor(actor)
ren1.AddActor(actor1)
renWin.SetSize(340, 400)

iren.Initialize()

# render the image
#iren.Start()
