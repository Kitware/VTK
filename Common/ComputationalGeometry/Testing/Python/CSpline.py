#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
math = vtk.vtkMath()
numberOfInputPoints = 30
aSplineX = vtk.vtkCardinalSpline()
aSplineY = vtk.vtkCardinalSpline()
aSplineZ = vtk.vtkCardinalSpline()
# generate random points
inputPoints = vtk.vtkPoints()
i = 0
while i < numberOfInputPoints:
    x = math.Random(0,1)
    y = math.Random(0,1)
    z = math.Random(0,1)
    aSplineX.AddPoint(i,x)
    aSplineY.AddPoint(i,y)
    aSplineZ.AddPoint(i,z)
    inputPoints.InsertPoint(i,x,y,z)
    i = i + 1

inputData = vtk.vtkPolyData()
inputData.SetPoints(inputPoints)
balls = vtk.vtkSphereSource()
balls.SetRadius(.01)
balls.SetPhiResolution(10)
balls.SetThetaResolution(10)
glyphPoints = vtk.vtkGlyph3D()
glyphPoints.SetInputData(inputData)
glyphPoints.SetSourceConnection(balls.GetOutputPort())
glyphMapper = vtk.vtkPolyDataMapper()
glyphMapper.SetInputConnection(glyphPoints.GetOutputPort())
glyph = vtk.vtkActor()
glyph.SetMapper(glyphMapper)
glyph.GetProperty().SetDiffuseColor(1,0.4,0.4)
glyph.GetProperty().SetSpecular(.3)
glyph.GetProperty().SetSpecularPower(30)
ren1.AddActor(glyph)
# create a polyline
points = vtk.vtkPoints()
profileData = vtk.vtkPolyData()
numberOfOutputPoints = 400
offset = 1.0
def fit (__vtk__temp0=0,__vtk__temp1=0):
    global numberOfInputPoints, numberOfOutputPoints, offset
    points.Reset()
    i = 0
    while i < numberOfOutputPoints:
        t = expr.expr(globals(), locals(),["(","numberOfInputPoints","-","offset",")","/","(","numberOfOutputPoints","-","1",")","*","i"])
        points.InsertPoint(i,aSplineX.Evaluate(t),aSplineY.Evaluate(t),aSplineZ.Evaluate(t))
        i = i + 1

    profileData.Modified()

fit()
lines = vtk.vtkCellArray()
lines.InsertNextCell(numberOfOutputPoints)
i = 0
while i < numberOfOutputPoints:
    lines.InsertCellPoint(i)
    i = i + 1

profileData.SetPoints(points)
profileData.SetLines(lines)
profileTubes = vtk.vtkTubeFilter()
profileTubes.SetNumberOfSides(8)
profileTubes.SetInputData(profileData)
profileTubes.SetRadius(.005)
profileMapper = vtk.vtkPolyDataMapper()
profileMapper.SetInputConnection(profileTubes.GetOutputPort())
profile = vtk.vtkActor()
profile.SetMapper(profileMapper)
profile.GetProperty().SetDiffuseColor(1,1,0.6)
profile.GetProperty().SetSpecular(.3)
profile.GetProperty().SetSpecularPower(30)
ren1.AddActor(profile)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()
renWin.SetSize(400,400)
# render the image
#
iren.Initialize()
def opened (__vtk__temp0=0,__vtk__temp1=0):
    global offset
    offset = 1.0
    aSplineX.ClosedOff()
    aSplineY.ClosedOff()
    aSplineZ.ClosedOff()
    fit()
    renWin.Render()

def varyLeft (__vtk__temp0=0,__vtk__temp1=0):
    left = -1
    while left <= 1:
        aSplineX.SetLeftValue(left)
        aSplineY.SetLeftValue(left)
        aSplineZ.SetLeftValue(left)
        fit()
        renWin.Render()
        left = expr.expr(globals(), locals(),["left","+",".05"])


def varyRight (__vtk__temp0=0,__vtk__temp1=0):
    right = -1
    while right <= 1:
        aSplineX.SetRightValue(right)
        aSplineY.SetRightValue(right)
        aSplineZ.SetRightValue(right)
        fit()
        renWin.Render()
        right = expr.expr(globals(), locals(),["right","+",".05"])


def constraint (value,__vtk__temp0=0,__vtk__temp1=0):
    aSplineX.SetLeftConstraint(value)
    aSplineY.SetLeftConstraint(value)
    aSplineZ.SetLeftConstraint(value)
    aSplineX.SetRightConstraint(value)
    aSplineY.SetRightConstraint(value)
    aSplineZ.SetRightConstraint(value)

def closed (__vtk__temp0=0,__vtk__temp1=0):
    global offset
    offset = 0.0
    aSplineX.ClosedOn()
    aSplineY.ClosedOn()
    aSplineZ.ClosedOn()
    fit()
    renWin.Render()

# prevent the tk window from showing up then start the event loop
# --- end of script --
