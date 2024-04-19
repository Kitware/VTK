#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkPolyDataNormals,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractVectorComponents
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
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
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
vx = vtkExtractVectorComponents()
vx.SetInputData(output)
vx.Update()
isoVx = vtkContourFilter()
isoVx.SetInputData(vx.GetVxComponent())
isoVx.SetValue(0,.38)
normalsVx = vtkPolyDataNormals()
normalsVx.SetInputConnection(isoVx.GetOutputPort())
normalsVx.SetFeatureAngle(45)
isoVxMapper = vtkPolyDataMapper()
isoVxMapper.SetInputConnection(normalsVx.GetOutputPort())
isoVxMapper.ScalarVisibilityOff()
isoVxActor = vtkActor()
isoVxActor.SetMapper(isoVxMapper)
isoVxActor.GetProperty().SetColor(1,0.7,0.6)
vy = vtkExtractVectorComponents()
vy.SetInputData(output)
vy.Update()
isoVy = vtkContourFilter()
isoVy.SetInputData(vy.GetVyComponent())
isoVy.SetValue(0,.38)
normalsVy = vtkPolyDataNormals()
normalsVy.SetInputConnection(isoVy.GetOutputPort())
normalsVy.SetFeatureAngle(45)
isoVyMapper = vtkPolyDataMapper()
isoVyMapper.SetInputConnection(normalsVy.GetOutputPort())
isoVyMapper.ScalarVisibilityOff()
isoVyActor = vtkActor()
isoVyActor.SetMapper(isoVyMapper)
isoVyActor.GetProperty().SetColor(0.7,1,0.6)
vz = vtkExtractVectorComponents()
vz.SetInputData(output)
vz.Update()
isoVz = vtkContourFilter()
isoVz.SetInputData(vz.GetVzComponent())
isoVz.SetValue(0,.38)
normalsVz = vtkPolyDataNormals()
normalsVz.SetInputConnection(isoVz.GetOutputPort())
normalsVz.SetFeatureAngle(45)
isoVzMapper = vtkPolyDataMapper()
isoVzMapper.SetInputConnection(normalsVz.GetOutputPort())
isoVzMapper.ScalarVisibilityOff()
isoVzActor = vtkActor()
isoVzActor.SetMapper(isoVzMapper)
isoVzActor.GetProperty().SetColor(0.4,0.5,1)
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoVxActor)
isoVxActor.AddPosition(0,12,0)
ren1.AddActor(isoVyActor)
ren1.AddActor(isoVzActor)
isoVzActor.AddPosition(0,-12,0)
ren1.SetBackground(.8,.8,.8)
renWin.SetSize(320,320)
ren1.GetActiveCamera().SetPosition(-63.3093,-1.55444,64.3922)
ren1.GetActiveCamera().SetFocalPoint(8.255,0.0499763,29.7631)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.ResetCameraClippingRange()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
