#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

math = vtkMath()
numberOfInputPoints=30

aSplineX = vtkKochanekSpline()
aSplineY = vtkKochanekSpline()
aSplineZ = vtkKochanekSpline()

# generate random points

inputPoints = vtkPoints()

for i in range(0,numberOfInputPoints):
	x=math.Random(0,1)
	y=math.Random(0,1)
	z=math.Random(0,1)
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
glyph.GetProperty().SetDiffuseColor(tomato)
glyph.GetProperty().SetSpecular(.3)
glyph.GetProperty().SetSpecularPower(30)

ren.AddActor(glyph)


points = vtkPoints()
# create a line
tension=0
bias=0
continuity=0
aSplineX.SetDefaultTension(tension)
aSplineX.SetDefaultBias(bias)
aSplineX.SetDefaultContinuity(continuity)
aSplineY.SetDefaultTension(tension)
aSplineY.SetDefaultBias(bias)
aSplineY.SetDefaultContinuity(continuity)
aSplineZ.SetDefaultTension(tension)
aSplineZ.SetDefaultBias(bias)
aSplineZ.SetDefaultContinuity(continuity)

profileData = vtkPolyData()
numberOfOutputPoints=300

def fit():
	global numberOfInputPoints, numberOfOutputPoints
	points.Reset()
	for i in range(0,numberOfOutputPoints):
		t= (numberOfInputPoints-1.0)/numberOfOutputPoints* i
		points.InsertPoint(i,aSplineX.Evaluate(t),aSplineY.Evaluate(t),aSplineZ.Evaluate(t))
   
	profileData.Modified()
 
fit()

lines = vtkCellArray()
lines.InsertNextCell(numberOfOutputPoints)

for i in range(0,numberOfOutputPoints):
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
profile.GetProperty().SetDiffuseColor(banana)
profile.GetProperty().SetSpecular(.3)
profile.GetProperty().SetSpecularPower(30)

ren.AddActor(profile)
ren.GetActiveCamera().Dolly(1.5)
ren.ResetCameraClippingRange()
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()

def defaults():
	aSplineX.SetDefaultBias(0)
	aSplineX.SetDefaultTension(0)
	aSplineX.SetDefaultContinuity(0)
	aSplineY.SetDefaultBias(0)
	aSplineY.SetDefaultTension(0)
	aSplineY.SetDefaultContinuity(0)
	aSplineZ.SetDefaultBias(0)
	aSplineZ.SetDefaultTension(0)
	aSplineZ.SetDefaultContinuity(0)
	fit()
	renWin.Render()
 
def varyBias():
	defaults
	bias = -1.0
	while bias <= 1.0:
		aSplineX.SetDefaultBias(bias)
		aSplineY.SetDefaultBias(bias)
		aSplineZ.SetDefaultBias(bias)
		fit
		renWin.Render()
		bias = bias + 0.05
     
 
def varyTension():
	defaults()
	tension = -1.0
	while tension <= 1.0:
		aSplineX.SetDefaultTension(tension)
		aSplineY.SetDefaultTension(tension)
		aSplineZ.SetDefaultTension(tension)
		fit()
		renWin.Render()
		tension = tension + 0.05
     
 
def varyContinuity():
	defaults()
	Continuity = -1.0
	while Continuity <= 1.0:
		aSplineX.SetDefaultContinuity(Continuity)
		aSplineY.SetDefaultContinuity(Continuity)
		aSplineZ.SetDefaultContinuity(Continuity)
		fit()
		renWin.Render()
		Continuity = Continuity + 0.05
     
 
renWin.SetFileName("KSpline.ppm")
iren.Start()
