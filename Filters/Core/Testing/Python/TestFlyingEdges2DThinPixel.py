#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkImageData
from vtkmodules.vtkFiltersCore import vtkFlyingEdges2D
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: a volume with a single voxel
im = vtkImageData()
im.SetDimensions(2,2,1)
im.AllocateScalars(10,1)

s = im.GetPointData().GetScalars()
s.InsertTuple1(0,-1.0)
s.InsertTuple1(1,-1.0)
s.InsertTuple1(2, 1.0)
s.InsertTuple1(3, 1.0)

iso = vtkFlyingEdges2D()
iso.SetInputData(im)
iso.SetValue(0,0.0)
iso.Update()

isoMapper = vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(1,0,0)
isoActor.GetProperty().SetOpacity(1)

outline = vtkOutlineFilter()
outline.SetInputData(im)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp = outlineActor.GetProperty()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoActor)

ren1.SetBackground(0,0,0)
renWin.SetSize(300,300)
ren1.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
