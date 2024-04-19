#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkImagingHybrid import vtkShepardMethod
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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
# create some points
#
math = vtkMath()
points = vtkPoints()
i = 0
while i < 50:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),math.Random(0,1))
    i = i + 1

scalars = vtkFloatArray()
i = 0
while i < 50:
    scalars.InsertValue(i,math.Random(0,1))
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars)
# triangulate them
#
shepard = vtkShepardMethod()
shepard.SetInputData(profile)
shepard.SetModelBounds(0,1,0,1,.1,.5)
#    shepard SetMaximumDistance .1
shepard.SetNullValue(1)
shepard.SetSampleDimensions(20,20,20)
shepard.Update()
map = vtkDataSetMapper()
map.SetInputConnection(shepard.GetOutputPort())
block = vtkActor()
block.SetMapper(map)
block.GetProperty().SetColor(1,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(block)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.Azimuth(160)
cam1.Elevation(30)
cam1.Zoom(1.5)
ren1.ResetCameraClippingRange()
renWin.Render()
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
threshold = 15
# --- end of script --
#iren.Start()
