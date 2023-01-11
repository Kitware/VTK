#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkImageData,
    vtkPlane,
)
from vtkmodules.vtkFiltersCore import vtkFlyingEdgesPlaneCutter
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

plane = vtkPlane()
plane.SetOrigin(0.5,0.5,0.5)
plane.SetNormal(1,1,1)

iso = vtkFlyingEdgesPlaneCutter()
iso.SetInputData(im)
iso.InterpolateAttributesOn()
iso.SetPlane(plane)
iso.Update()

isoMapper = vtkPolyDataMapper()
isoMapper.SetInputConnection(iso.GetOutputPort())
isoMapper.SetScalarRange(iso.GetOutput().GetPointData().GetScalars().GetRange())

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
