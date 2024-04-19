#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkMergePoints
from vtkmodules.vtkFiltersCore import vtkMarchingSquares
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOImage import vtkVolume16Reader
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
v16 = vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
v16.SetImageRange(1,93)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()
myLocator = vtkMergePoints()
isoXY = vtkMarchingSquares()
isoXY.SetInputConnection(v16.GetOutputPort())
isoXY.GenerateValues(2,600,1200)
isoXY.SetImageRange(0,32,32,63,45,45)
isoXY.SetLocator(myLocator)
isoXYMapper = vtkPolyDataMapper()
isoXYMapper.SetInputConnection(isoXY.GetOutputPort())
isoXYMapper.SetScalarRange(600,1200)
isoXYActor = vtkActor()
isoXYActor.SetMapper(isoXYMapper)
isoYZ = vtkMarchingSquares()
isoYZ.SetInputConnection(v16.GetOutputPort())
isoYZ.GenerateValues(2,600,1200)
isoYZ.SetImageRange(32,32,32,63,46,92)
isoYZMapper = vtkPolyDataMapper()
isoYZMapper.SetInputConnection(isoYZ.GetOutputPort())
isoYZMapper.SetScalarRange(600,1200)
isoYZActor = vtkActor()
isoYZActor.SetMapper(isoYZMapper)
isoXZ = vtkMarchingSquares()
isoXZ.SetInputConnection(v16.GetOutputPort())
isoXZ.GenerateValues(2,600,1200)
isoXZ.SetImageRange(0,32,32,32,0,46)
isoXZMapper = vtkPolyDataMapper()
isoXZMapper.SetInputConnection(isoXZ.GetOutputPort())
isoXZMapper.SetScalarRange(600,1200)
isoXZActor = vtkActor()
isoXZActor.SetMapper(isoXZMapper)
outline = vtkOutlineFilter()
outline.SetInputConnection(v16.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(isoXYActor)
ren1.AddActor(isoYZActor)
ren1.AddActor(isoXZActor)
ren1.SetBackground(0.9,.9,.9)
renWin.SetSize(200,200)
ren1.GetActiveCamera().SetPosition(324.368,284.266,-19.3293)
ren1.GetActiveCamera().SetFocalPoint(73.5683,120.903,70.7309)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(-0.304692,-0.0563843,-0.950781)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
