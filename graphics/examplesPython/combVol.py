#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


from colors import *
# Create the RenderWindow, Renderer and Interactor
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkPlane()
plane.SetOrigin(0,4,2)
plane.SetNormal(0,1,0)
cutter = vtkCutter()
cutter.SetInput(pl3d.GetOutput())
cutter.SetCutFunction(plane)
cutter.GenerateCutScalarsOff()
cutter.SetSortBy(1)

clut = vtkLookupTable()
clut.SetHueRange(0,.67)
clut.Build()

cutterMapper = vtkPolyDataMapper()
cutterMapper.SetInput(cutter.GetOutput())
cutterMapper.SetScalarRange(.18,.7)
cutterMapper.SetLookupTable(clut)

cut = vtkActor()
cut.SetMapper(cutterMapper)

iso = vtkContourFilter()
iso.SetInput(pl3d.GetOutput())
iso.SetValue(0,.22)
normals = vtkPolyDataNormals()
normals.SetInput(iso.GetOutput())
normals.SetFeatureAngle(45)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(normals.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetDiffuseColor(tomato)
isoActor.GetProperty().SetSpecularColor(white)
isoActor.GetProperty().SetDiffuse(.8)
isoActor.GetProperty().SetSpecular(.5)
isoActor.GetProperty().SetSpecularPower(30)

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineTubes = vtkTubeFilter()
outlineTubes.SetInput(outline.GetOutput())
outlineTubes.SetRadius(.1)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outlineTubes.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
outlineActor.GetProperty().SetColor(banana[0],banana[1],banana[2])
ren.AddActor(isoActor)
isoActor.VisibilityOn()
ren.AddActor(cut)
cut.GetProperty().SetOpacity(.06)
ren.SetBackground(1,1,1)
renWin.SetSize(640,480)

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(9.71821,0.458166,29.3999)
cam1.SetPosition(2.7439,-37.3196,38.7167)
cam1.SetViewUp(-0.16123,0.264271,0.950876)

# render the image
#

#
# Cut: generates n cut planes normal to camera's view plane
#
def Cut(n):
	global cam1
	plane.SetNormal(cam1.GetViewPlaneNormal())
	plane.SetOrigin(cam1.GetFocalPoint())
	cutter.GenerateValues(n,-15,15)
	clut.SetAlphaRange(cut.GetProperty().GetOpacity(), cut.GetProperty().GetOpacity())
	renWin.Render()
 

# Generate 10 cut planes
Cut(10)

iren.Start()
