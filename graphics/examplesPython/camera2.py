#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# create camera figure

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a camera model
camCS = vtkConeSource()
camCS.SetHeight(1.5)
camCS.SetResolution(12)
camCS.SetRadius(0.4)

camCBS = vtkCubeSource()
camCBS.SetXLength(1.5)
camCBS.SetZLength(0.8)
camCBS.SetCenter(0.4,0,0)

camAPD = vtkAppendPolyData()
camAPD.AddInput(camCBS.GetOutput())
camAPD.AddInput(camCS.GetOutput())

camMapper = vtkPolyDataMapper()
camMapper.SetInput(camAPD.GetOutput())
camActor = vtkLODActor()
camActor.SetMapper(camMapper)
camActor.SetScale(2,2,2)

# draw the arrows
pd = vtkPolyData()
ca = vtkCellArray()
fp = vtkPoints()
fp.InsertNextPoint(0,1,0)
fp.InsertNextPoint(8,1,0)
fp.InsertNextPoint(8,2,0)
fp.InsertNextPoint(10,0,0)
fp.InsertNextPoint(8,-2,0)
fp.InsertNextPoint(8,-1,0)
fp.InsertNextPoint(0,-1,0)
ca.InsertNextCell(7)
ca.InsertCellPoint(0)
ca.InsertCellPoint(1)
ca.InsertCellPoint(2)
ca.InsertCellPoint(3)
ca.InsertCellPoint(4)
ca.InsertCellPoint(5)
ca.InsertCellPoint(6)
pd.SetPoints(fp)
pd.SetPolys(ca)

pd2 = vtkPolyData()
ca2 = vtkCellArray()
fp2 = vtkPoints()
fp2.InsertNextPoint(0,1,0)
fp2.InsertNextPoint(8,1,0)
fp2.InsertNextPoint(8,2,0)
fp2.InsertNextPoint(10,0.01,0) #prevents.degenerate.triangles
ca2.InsertNextCell(4)
ca2.InsertCellPoint(0)
ca2.InsertCellPoint(1)
ca2.InsertCellPoint(2)
ca2.InsertCellPoint(3)
pd2.SetPoints(fp2)
pd2.SetLines(ca2)

arrowIM = vtkImplicitModeller()
arrowIM.SetInput(pd)
arrowIM.SetSampleDimensions(50,20,8)

arrowCF = vtkContourFilter()
arrowCF.SetInput(arrowIM.GetOutput())
arrowCF.SetValue(0,0.2)

arrowWT = vtkWarpTo()
arrowWT.SetInput(arrowCF.GetOutput())
arrowWT.SetPosition(5,0,5)
arrowWT.SetScaleFactor(0.85)
arrowWT.AbsoluteOn()

arrowT = vtkTransform()
arrowT.RotateY(60)
arrowT.Translate(-1.33198,0,-1.479)
arrowT.Scale(1,0.5,1)

arrowTF = vtkTransformFilter()
arrowTF.SetInput(arrowWT.GetOutput())
arrowTF.SetTransform(arrowT)

arrowMapper = vtkDataSetMapper()
arrowMapper.SetInput(arrowTF.GetOutput())
arrowMapper.ScalarVisibilityOff()

# draw the azimuth arrows
a1Actor = vtkLODActor()
a1Actor.SetMapper(arrowMapper)
a1Actor.SetPosition(-9,0,-1)
a1Actor.GetProperty().SetColor(1,0.3,0.3)
a1Actor.GetProperty().SetSpecularColor(1,1,1)
a1Actor.GetProperty().SetSpecular(0.3)
a1Actor.GetProperty().SetSpecularPower(20)
a1Actor.GetProperty().SetAmbient(0.2)
a1Actor.GetProperty().SetDiffuse(0.8)

a2Actor = vtkLODActor()
a2Actor.SetMapper(arrowMapper)
a2Actor.RotateX(180)
a2Actor.SetPosition(-9,0,1)
a2Actor.GetProperty().SetColor(1,0.3,0.3)
a2Actor.GetProperty().SetSpecularColor(1,1,1)
a2Actor.GetProperty().SetSpecular(0.3)
a2Actor.GetProperty().SetSpecularPower(20)
a2Actor.GetProperty().SetAmbient(0.2)
a2Actor.GetProperty().SetDiffuse(0.8)

# draw the elevation arrows
a3Actor = vtkLODActor()
a3Actor.SetMapper(arrowMapper)
a3Actor.RotateX(-90)
a3Actor.SetPosition(-9,-1,0)
a3Actor.GetProperty().SetColor(0.3,1,0.3)
a3Actor.GetProperty().SetSpecularColor(1,1,1)
a3Actor.GetProperty().SetSpecular(0.3)
a3Actor.GetProperty().SetSpecularPower(20)
a3Actor.GetProperty().SetAmbient(0.2)
a3Actor.GetProperty().SetDiffuse(0.8)

a4Actor = vtkLODActor()
a4Actor.SetMapper(arrowMapper)
a4Actor.RotateX(90)
a4Actor.SetPosition(-9,1,0)
a4Actor.GetProperty().SetColor(0.3,1,0.3)
a4Actor.GetProperty().SetSpecularColor(1,1,1)
a4Actor.GetProperty().SetSpecular(0.3)
a4Actor.GetProperty().SetSpecularPower(20)
a4Actor.GetProperty().SetAmbient(0.2)
a4Actor.GetProperty().SetDiffuse(0.8)

# draw the DOP
arrowT2 = vtkTransform()
arrowT2.Scale(1,0.6,1)
arrowT2.RotateY(90)

arrowTF2 = vtkTransformPolyDataFilter()
arrowTF2.SetInput(pd2)
arrowTF2.SetTransform(arrowT2)

arrowREF = vtkRotationalExtrusionFilter()
arrowREF.SetInput(arrowTF2.GetOutput())
arrowREF.CappingOff()
arrowREF.SetResolution(30)

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInput(arrowREF.GetOutput())

a5Actor = vtkLODActor()
a5Actor.SetMapper(spikeMapper)
a5Actor.SetScale(0.3,0.3,0.6)
a5Actor.RotateY(-90)
a5Actor.SetPosition(-8,0,0)
a5Actor.GetProperty().SetColor(1,0.3,1)
a5Actor.GetProperty().SetSpecularColor(1,1,1)
a5Actor.GetProperty().SetSpecular(0.3)
a5Actor.GetProperty().SetAmbient(0.2)
a5Actor.GetProperty().SetDiffuse(0.8)
a5Actor.GetProperty().SetSpecularPower(20)

a7Actor = vtkLODActor()
a7Actor.SetMapper(spikeMapper)
a7Actor.SetScale(0.2,0.2,0.7)
a7Actor.RotateZ(90)
a7Actor.RotateY(-90)
a7Actor.SetPosition(-9,1,0)
a7Actor.GetProperty().SetColor(0.3,1,1)
a7Actor.GetProperty().SetSpecularColor(1,1,1)
a7Actor.GetProperty().SetSpecular(0.3)
a7Actor.GetProperty().SetAmbient(0.2)
a7Actor.GetProperty().SetDiffuse(0.8)
a7Actor.GetProperty().SetSpecularPower(20)

# focal point
fps = vtkSphereSource()
fps.SetRadius(0.5)
fpMapper = vtkPolyDataMapper()
fpMapper.SetInput(fps.GetOutput())
fpActor = vtkLODActor()
fpActor.SetMapper(fpMapper)
fpActor.SetPosition(-9,0,0)
fpActor.GetProperty().SetSpecularColor(1,1,1)
fpActor.GetProperty().SetSpecular(0.3)
fpActor.GetProperty().SetAmbient(0.2)
fpActor.GetProperty().SetDiffuse(0.8)
fpActor.GetProperty().SetSpecularPower(20)

# create the roll arrows
arrowWT2 = vtkWarpTo()
arrowWT2.SetInput(arrowCF.GetOutput())
arrowWT2.SetPosition(5,0,2.5)
arrowWT2.SetScaleFactor(0.95)
arrowWT2.AbsoluteOn()

arrowT3 = vtkTransform()
arrowT3.Translate(-2.50358,0,-1.70408)
arrowT3.Scale(0.5,0.3,1)

arrowTF3 = vtkTransformFilter()
arrowTF3.SetInput(arrowWT2.GetOutput())
arrowTF3.SetTransform(arrowT3)

arrowMapper2 = vtkDataSetMapper()
arrowMapper2.SetInput(arrowTF3.GetOutput())
arrowMapper2.ScalarVisibilityOff()

# draw the roll arrows
a6Actor = vtkLODActor()
a6Actor.SetMapper(arrowMapper2)
a6Actor.RotateZ(90)
a6Actor.SetPosition(-4,0,0)
a6Actor.SetScale(1.5,1.5,1.5)
a6Actor.GetProperty().SetColor(1,1,0.3)
a6Actor.GetProperty().SetSpecularColor(1,1,1)
a6Actor.GetProperty().SetSpecular(0.3)
a6Actor.GetProperty().SetSpecularPower(20)
a6Actor.GetProperty().SetAmbient(0.2)
a6Actor.GetProperty().SetDiffuse(0.8)

# Add the actors to the renderer, set the background and size
ren.AddActor(camActor)
ren.AddActor(a1Actor)
ren.AddActor(a2Actor)
ren.AddActor(a3Actor)
ren.AddActor(a4Actor)
ren.AddActor(a5Actor)
ren.AddActor(a6Actor)
ren.AddActor(a7Actor)
ren.AddActor(fpActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
cam1=ren.GetActiveCamera()
cam1.Zoom(1.5)
cam1.Azimuth(150)
cam1.Elevation(30)

iren.Initialize()


iren.Start()
