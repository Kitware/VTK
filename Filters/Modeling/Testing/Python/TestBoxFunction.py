#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkBox
from vtkmodules.vtkFiltersCore import vtkContourFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOLegacy import vtkPolyDataWriter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
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

box = vtkBox()
box.SetXMin(0,2,4)
box.SetXMax(2,4,6)
sample = vtkSampleFunction()
sample.SetSampleDimensions(30,30,30)
sample.SetImplicitFunction(box)
sample.SetModelBounds(0,1.5,1,5,2,8)
sample.ComputeNormalsOn()
contours = vtkContourFilter()
contours.SetInputConnection(sample.GetOutputPort())
contours.GenerateValues(5,-0.5,1.5)
w = vtkPolyDataWriter()
w.SetInputConnection(contours.GetOutputPort())
w.SetFileName("junk.vtk")
#w Write
contMapper = vtkPolyDataMapper()
contMapper.SetInputConnection(contours.GetOutputPort())
contMapper.SetScalarRange(-0.5,1.5)
contActor = vtkActor()
contActor.SetMapper(contMapper)
# We'll put a simple outline around the data.
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# The usual rendering stuff.
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.SetSize(500,500)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.SetBackground(1,1,1)
ren1.AddActor(contActor)
ren1.AddActor(outlineActor)
camera = vtkCamera()
camera.SetClippingRange(6.31875,20.689)
camera.SetFocalPoint(0.75,3,5)
camera.SetPosition(9.07114,-4.10065,-1.38712)
camera.SetViewAngle(30)
camera.SetViewUp(-0.580577,-0.802756,0.13606)
ren1.SetActiveCamera(camera)
iren.Initialize()
# --- end of script --
