#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# get the interactor ui
# Now create the RenderWindow, Renderer and Interactor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
math = vtk.vtkMath()
numberOfInputPoints = 30
aKSplineX = vtk.vtkKochanekSpline()
aKSplineX.ClosedOn()
aKSplineY = vtk.vtkKochanekSpline()
aKSplineY.ClosedOn()
aKSplineZ = vtk.vtkKochanekSpline()
aKSplineZ.ClosedOn()
aCSplineX = vtk.vtkCardinalSpline()
aCSplineX.ClosedOn()
aCSplineY = vtk.vtkCardinalSpline()
aCSplineY.ClosedOn()
aCSplineZ = vtk.vtkCardinalSpline()
aCSplineZ.ClosedOn()
# add some points
inputPoints = vtk.vtkPoints()
x = -1.0
y = -1.0
z = 0.0
aKSplineX.AddPoint(0,x)
aKSplineY.AddPoint(0,y)
aKSplineZ.AddPoint(0,z)
aCSplineX.AddPoint(0,x)
aCSplineY.AddPoint(0,y)
aCSplineZ.AddPoint(0,z)
inputPoints.InsertPoint(0,x,y,z)
x = 1.0
y = -1.0
z = 0.0
aKSplineX.AddPoint(1,x)
aKSplineY.AddPoint(1,y)
aKSplineZ.AddPoint(1,z)
aCSplineX.AddPoint(1,x)
aCSplineY.AddPoint(1,y)
aCSplineZ.AddPoint(1,z)
inputPoints.InsertPoint(1,x,y,z)
x = 1.0
y = 1.0
z = 0.0
aKSplineX.AddPoint(2,x)
aKSplineY.AddPoint(2,y)
aKSplineZ.AddPoint(2,z)
aCSplineX.AddPoint(2,x)
aCSplineY.AddPoint(2,y)
aCSplineZ.AddPoint(2,z)
inputPoints.InsertPoint(2,x,y,z)
x = -1.0
y = 1.0
z = 0.0
aKSplineX.AddPoint(3,x)
aKSplineY.AddPoint(3,y)
aKSplineZ.AddPoint(3,z)
aCSplineX.AddPoint(3,x)
aCSplineY.AddPoint(3,y)
aCSplineZ.AddPoint(3,z)
inputPoints.InsertPoint(3,x,y,z)
inputData = vtk.vtkPolyData()
inputData.SetPoints(inputPoints)
balls = vtk.vtkSphereSource()
balls.SetRadius(.04)
balls.SetPhiResolution(10)
balls.SetThetaResolution(10)
glyphPoints = vtk.vtkGlyph3D()
glyphPoints.SetInputData(inputData)
glyphPoints.SetSourceConnection(balls.GetOutputPort())
glyphMapper = vtk.vtkPolyDataMapper()
glyphMapper.SetInputConnection(glyphPoints.GetOutputPort())
glyph = vtk.vtkActor()
glyph.SetMapper(glyphMapper)
glyph.GetProperty().SetDiffuseColor(tomato)
glyph.GetProperty().SetSpecular(.3)
glyph.GetProperty().SetSpecularPower(30)
ren1.AddActor(glyph)
Kpoints = vtk.vtkPoints()
Cpoints = vtk.vtkPoints()
profileKData = vtk.vtkPolyData()
profileCData = vtk.vtkPolyData()
numberOfInputPoints = 5
numberOfOutputPoints = 100
offset = 1.0
def fit (__vtk__temp0=0,__vtk__temp1=0):
    global numberOfInputPoints, numberOfOutputPoints, offset
    Kpoints.Reset()
    Cpoints.Reset()
    i = 0
    while i < numberOfOutputPoints:
        t = expr.expr(globals(), locals(),["(","numberOfInputPoints","-","offset",")","/","(","numberOfOutputPoints","-","1",")","*","i"])
        Kpoints.InsertPoint(i,aKSplineX.Evaluate(t),aKSplineY.Evaluate(t),aKSplineZ.Evaluate(t))
        Cpoints.InsertPoint(i,aCSplineX.Evaluate(t),aCSplineY.Evaluate(t),aCSplineZ.Evaluate(t))
        i = i + 1

    profileKData.Modified()
    profileCData.Modified()

fit()
lines = vtk.vtkCellArray()
lines.InsertNextCell(numberOfOutputPoints)
i = 0
while i < numberOfOutputPoints:
    lines.InsertCellPoint(i)
    i = i + 1

profileKData.SetPoints(Kpoints)
profileKData.SetLines(lines)
profileCData.SetPoints(Cpoints)
profileCData.SetLines(lines)
profileKTubes = vtk.vtkTubeFilter()
profileKTubes.SetNumberOfSides(8)
profileKTubes.SetInputData(profileKData)
profileKTubes.SetRadius(.01)
profileKMapper = vtk.vtkPolyDataMapper()
profileKMapper.SetInputConnection(profileKTubes.GetOutputPort())
profileK = vtk.vtkActor()
profileK.SetMapper(profileKMapper)
profileK.GetProperty().SetDiffuseColor(banana)
profileK.GetProperty().SetSpecular(.3)
profileK.GetProperty().SetSpecularPower(30)
ren1.AddActor(profileK)
profileCTubes = vtk.vtkTubeFilter()
profileCTubes.SetNumberOfSides(8)
profileCTubes.SetInputData(profileCData)
profileCTubes.SetRadius(.01)
profileCMapper = vtk.vtkPolyDataMapper()
profileCMapper.SetInputConnection(profileCTubes.GetOutputPort())
profileC = vtk.vtkActor()
profileC.SetMapper(profileCMapper)
profileC.GetProperty().SetDiffuseColor(peacock)
profileC.GetProperty().SetSpecular(.3)
profileC.GetProperty().SetSpecularPower(30)
ren1.AddActor(profileC)
ren1.ResetCamera()
ren1.GetActiveCamera().Dolly(1.5)
ren1.ResetCameraClippingRange()
renWin.SetSize(300,300)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
def defaults (__vtk__temp0=0,__vtk__temp1=0):
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

def varyBias (__vtk__temp0=0,__vtk__temp1=0):
    defaults()
    bias = -1
    while bias <= 1:
        aSplineX.SetDefaultBias(bias)
        aSplineY.SetDefaultBias(bias)
        aSplineZ.SetDefaultBias(bias)
        fit()
        renWin.Render()
        bias = expr.expr(globals(), locals(),["bias","+",".05"])


def varyTension (__vtk__temp0=0,__vtk__temp1=0):
    defaults()
    tension = -1
    while tension <= 1:
        aSplineX.SetDefaultTension(tension)
        aSplineY.SetDefaultTension(tension)
        aSplineZ.SetDefaultTension(tension)
        fit()
        renWin.Render()
        tension = expr.expr(globals(), locals(),["tension","+",".05"])


def varyContinuity (__vtk__temp0=0,__vtk__temp1=0):
    defaults()
    Continuity = -1
    while Continuity <= 1:
        aSplineX.SetDefaultContinuity(Continuity)
        aSplineY.SetDefaultContinuity(Continuity)
        aSplineZ.SetDefaultContinuity(Continuity)
        fit()
        renWin.Render()
        Continuity = expr.expr(globals(), locals(),["Continuity","+",".05"])


def closed (__vtk__temp0=0,__vtk__temp1=0):
    global offset
    offset = 0.0
    aSplineX.ClosedOn()
    aSplineY.ClosedOn()
    aSplineZ.ClosedOn()
    fit()
    renWin.Render()

def opened (__vtk__temp0=0,__vtk__temp1=0):
    global offset
    offset = 1.0
    aSplineX.ClosedOff()
    aSplineY.ClosedOff()
    aSplineZ.ClosedOff()
    fit()
    renWin.Render()

# --- end of script --
