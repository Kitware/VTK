#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'
from libVTKCommonPython import *
from libVTKGraphicsPython import *

import whrandom

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

numberOfInputPoints = 30

aSplineX = vtkCardinalSpline()
aSplineY = vtkCardinalSpline()
aSplineZ = vtkCardinalSpline()

# generate random points

inputPoints = vtkPoints()

for i in range(numberOfInputPoints):
    x = whrandom.random()
    y = whrandom.random()
    z = whrandom.random()
    aSplineX.AddPoint(i,x)
    aSplineY.AddPoint(i,y)
    aSplineZ.AddPoint(i,z)
    inputPoints.InsertPoint(i,x,y,z)

inputData = vtkPolyData()
inputData.SetPoints(inputPoints)

balls = vtkSphereSource()
balls.SetRadius(.01)
balls.SetPhiResolution(10)
balls.SetThetaResolution(10)

glyphPoints = vtkGlyph3D()
glyphPoints.SetInput(inputData)
glyphPoints.SetSource(balls.GetOutput())

glyphMapper = vtkPolyDataMapper()
glyphMapper.SetInput(glyphPoints.GetOutput())

glyph = vtkActor()
glyph.SetMapper(glyphMapper)
prop = glyph.GetProperty()
#prop.SetDiffuseColor(vtkColors.tomato)
prop.SetSpecular(.3)
prop.SetSpecularPower(30)

ren1.AddActor(glyph)

points = vtkPoints()

profileData = vtkPolyData()

numberOfOutputPoints = 400

def fit():
    points.Reset()
    for i in range(numberOfOutputPoints):
	t = (numberOfInputPoints - 1.0) / (numberOfOutputPoints - 1) * i
	points.InsertPoint(i, aSplineX.Evaluate(t),
			   aSplineY.Evaluate(t),
			   aSplineZ.Evaluate(t))
    profileData.Modified()

fit()

lines = vtkCellArray()
lines.InsertNextCell(numberOfOutputPoints)

for i in range(numberOfOutputPoints):
    lines.InsertCellPoint(i)

profileData.SetPoints(points)
profileData.SetLines(lines)

profileTubes = vtkTubeFilter()
profileTubes.SetNumberOfSides(8)
profileTubes.SetInput(profileData)
profileTubes.SetRadius(.005)

profileMapper = vtkPolyDataMapper()
profileMapper.SetInput(profileTubes.GetOutput())

profile = vtkActor()
profile.SetMapper(profileMapper)

prop = profile.GetProperty()
#prop.SetDiffuseColor(vtkColors.banana)
prop.SetSpecular(.3)
prop.SetSpecularPower(30)

ren1.AddActor(profile)

ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()

###iren.SetUserMethod(lambda: wm.deiconify(.vtkInteract))

def varyLeft():
    left = -1
    while left <= 1:
	aSplineX.SetLeftValue(left)
	aSplineY.SetLeftValue(left)
	aSplineZ.SetLeftValue(left)
	fit()
	renWin.Render()
	left = left + 0.05

def varyRight():
    right = -1
    while right <= 1:
	aSplineX.SetRightValue(right)
	aSplineY.SetRightValue(right)
	aSplineZ.SetRightValue(right)
	fit()
	renWin.Render()
	right = right + 0.05

def constraint(value):
    aSplineX.SetLeftConstraint(value)
    aSplineX.SetRightConstraint(value)
    aSplineY.SetLeftConstraint(value)
    aSplineY.SetRightConstraint(value)
    aSplineZ.SetLeftConstraint(value)
    aSplineZ.SetRightConstraint(value)

renWin.SetFileName("CSpline.ppm")
iren.Start()

