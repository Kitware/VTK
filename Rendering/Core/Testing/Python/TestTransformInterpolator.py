#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkPoints,
    vtkUnsignedCharArray,
)
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTransformInterpolator,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create two boxes and interpolate between them
#
pts = vtkPoints()
pts.InsertNextPoint(-1,-1,-1)
pts.InsertNextPoint(1,-1,-1)
pts.InsertNextPoint(1,1,-1)
pts.InsertNextPoint(-1,1,-1)
pts.InsertNextPoint(-1,-1,1)
pts.InsertNextPoint(1,-1,1)
pts.InsertNextPoint(1,1,1)
pts.InsertNextPoint(-1,1,1)
faces = vtkCellArray()
faces.InsertNextCell(4)
faces.InsertCellPoint(0)
faces.InsertCellPoint(3)
faces.InsertCellPoint(2)
faces.InsertCellPoint(1)
faces.InsertNextCell(4)
faces.InsertCellPoint(4)
faces.InsertCellPoint(5)
faces.InsertCellPoint(6)
faces.InsertCellPoint(7)
faces.InsertNextCell(4)
faces.InsertCellPoint(0)
faces.InsertCellPoint(1)
faces.InsertCellPoint(5)
faces.InsertCellPoint(4)
faces.InsertNextCell(4)
faces.InsertCellPoint(1)
faces.InsertCellPoint(2)
faces.InsertCellPoint(6)
faces.InsertCellPoint(5)
faces.InsertNextCell(4)
faces.InsertCellPoint(2)
faces.InsertCellPoint(3)
faces.InsertCellPoint(7)
faces.InsertCellPoint(6)
faces.InsertNextCell(4)
faces.InsertCellPoint(3)
faces.InsertCellPoint(0)
faces.InsertCellPoint(4)
faces.InsertCellPoint(7)
faceColors = vtkUnsignedCharArray()
faceColors.SetNumberOfComponents(3)
faceColors.SetNumberOfTuples(3)
faceColors.InsertComponent(0,0,255)
faceColors.InsertComponent(0,1,0)
faceColors.InsertComponent(0,2,0)
faceColors.InsertComponent(1,0,0)
faceColors.InsertComponent(1,1,255)
faceColors.InsertComponent(1,2,0)
faceColors.InsertComponent(2,0,255)
faceColors.InsertComponent(2,1,255)
faceColors.InsertComponent(2,2,0)
faceColors.InsertComponent(3,0,0)
faceColors.InsertComponent(3,1,0)
faceColors.InsertComponent(3,2,255)
faceColors.InsertComponent(4,0,255)
faceColors.InsertComponent(4,1,0)
faceColors.InsertComponent(4,2,255)
faceColors.InsertComponent(5,0,0)
faceColors.InsertComponent(5,1,255)
faceColors.InsertComponent(5,2,255)
cube = vtkPolyData()
cube.SetPoints(pts)
cube.SetPolys(faces)
cube.GetCellData().SetScalars(faceColors)
t1 = vtkTransform()
t1.Translate(1,2,3)
t1.RotateX(15)
t1.Scale(4,2,1)
tpdf1 = vtkTransformPolyDataFilter()
tpdf1.SetInputData(cube)
tpdf1.SetTransform(t1)
cube1Mapper = vtkPolyDataMapper()
cube1Mapper.SetInputConnection(tpdf1.GetOutputPort())
cube1 = vtkActor()
cube1.SetMapper(cube1Mapper)
t2 = vtkTransform()
t2.Translate(5,10,15)
t2.RotateX(22.5)
t2.RotateY(15)
t2.RotateZ(85)
t2.Scale(1,2,4)
tpdf2 = vtkTransformPolyDataFilter()
tpdf2.SetInputData(cube)
tpdf2.SetTransform(t2)
cube2Mapper = vtkPolyDataMapper()
cube2Mapper.SetInputConnection(tpdf2.GetOutputPort())
cube2 = vtkActor()
cube2.SetMapper(cube2Mapper)
t3 = vtkTransform()
t3.Translate(5,-10,15)
t3.RotateX(13)
t3.RotateY(72)
t3.RotateZ(-15)
t3.Scale(2,4,1)
tpdf3 = vtkTransformPolyDataFilter()
tpdf3.SetInputData(cube)
tpdf3.SetTransform(t3)
cube3Mapper = vtkPolyDataMapper()
cube3Mapper.SetInputConnection(tpdf3.GetOutputPort())
cube3 = vtkActor()
cube3.SetMapper(cube3Mapper)
t4 = vtkTransform()
t4.Translate(10,-5,5)
t4.RotateX(66)
t4.RotateY(19)
t4.RotateZ(24)
t4.Scale(2,.5,1)
tpdf4 = vtkTransformPolyDataFilter()
tpdf4.SetInputData(cube)
tpdf4.SetTransform(t4)
cube4Mapper = vtkPolyDataMapper()
cube4Mapper.SetInputConnection(tpdf4.GetOutputPort())
cube4 = vtkActor()
cube4.SetMapper(cube4Mapper)
# Interpolate the transformation
cubeMapper = vtkPolyDataMapper()
cubeMapper.SetInputData(cube)
cubeActor = vtkActor()
cubeActor.SetMapper(cubeMapper)
# Interpolate some transformations, test along the way
interpolator = vtkTransformInterpolator()
#interpolator SetInterpolationTypeToLinear
interpolator.SetInterpolationTypeToSpline()
interpolator.AddTransform(0.0,cube1)
interpolator.AddTransform(8.0,cube2)
interpolator.AddTransform(18.2,cube3)
interpolator.AddTransform(24.4,cube4)
interpolator.Initialize()
#puts [interpolator GetNumberOfTransforms]
interpolator.AddTransform(0.0,t1)
interpolator.AddTransform(8.0,t2)
interpolator.AddTransform(18.2,t3)
interpolator.AddTransform(24.4,t4)
#puts [interpolator GetNumberOfTransforms]
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(cube1)
ren1.AddActor(cube2)
ren1.AddActor(cube3)
ren1.AddActor(cube4)
ren1.AddActor(cubeActor)
ren1.SetBackground(0,0,0)
renWin.SetSize(300,300)
ren1.SetBackground(0.1,0.2,0.4)
# render the image
#
camera = vtkCamera()
camera.SetClippingRange(31.2977,81.697)
camera.SetFocalPoint(3.0991,-2.00445,9.78648)
camera.SetPosition(-44.8481,-25.871,10.0645)
camera.SetViewAngle(30)
camera.SetViewUp(-0.0356378,0.0599728,-0.997564)
ren1.SetActiveCamera(camera)
renWin.Render()
# prevent the tk window from showing up then start the event loop
xform = vtkTransform()
def animate():
    numSteps = 250
    min = interpolator.GetMinimumT()
    max = interpolator.GetMaximumT()
    i = 0
    while i <= numSteps:
        t = float(i)*(max-min)/float(numSteps)
        interpolator.InterpolateTransform(t,xform)
        cubeActor.SetUserMatrix(xform.GetMatrix())
        renWin.Render()
        i = i + 1


interpolator.InterpolateTransform(13.2,xform)
cubeActor.SetUserMatrix(xform.GetMatrix())
renWin.Render()
#animate()
# --- end of script --
