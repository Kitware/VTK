#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersGeneral import vtkClipDataSet
from vtkmodules.vtkFiltersGeometry import vtkDataSetSurfaceFilter
from vtkmodules.vtkIOExodus import vtkExodusIIReader
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

# Create the RenderWindow, Renderer
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer( ren )
renWin.SetSize(600,200)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Read input dataset that has n-faced polyhedra
reader = vtkExodusIIReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/cube-1.exo")
reader.Update()
dataset = reader.GetOutput()

# clip the dataset
clipper = vtkClipDataSet()
clipper.SetInputData(dataset.GetBlock(0).GetBlock(0))
plane = vtkPlane()
plane.SetNormal(0.5,0.5,0.5)
plane.SetOrigin(0.5,0.5,0.5)
clipper.SetClipFunction(plane)
clipper.Update()

# get surface representation to render
surfaceFilter = vtkDataSetSurfaceFilter()
surfaceFilter.SetInputData(clipper.GetOutput())
surfaceFilter.Update()
surface = surfaceFilter.GetOutput()

mapper = vtkPolyDataMapper()
mapper.SetInputData(surfaceFilter.GetOutput())
mapper.Update()

actor = vtkActor()
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
