#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# motor visualization
from colors import *
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create cutting planes
planes = vtkPlanes()
points = vtkPoints()
norms = vtkNormals()

points.InsertPoint(0,0.0,0.0,0.0)
norms.InsertNormal(0,0.0,0.0,1.0)
points.InsertPoint(1,0.0,0.0,0.0)
norms.InsertNormal(1,-1.0,0.0,0.0)

planes.SetPoints(points)
planes.SetNormals(norms)

# texture
texReader = vtkStructuredPointsReader()
texReader.SetFileName(VTK_DATA + "/texThres2.vtk")
texture = vtkTexture()
texture.SetInput(texReader.GetOutput())
texture.InterpolateOff()
texture.RepeatOff()

# read motor parts...each part colored separately
#
byu = vtkBYUReader()
byu.SetGeometryFileName(VTK_DATA + "/motor.g")
byu.SetPartNumber(1)
normals = vtkPolyDataNormals()
normals.SetInput(byu.GetOutput())
tex1 = vtkImplicitTextureCoords()
tex1.SetInput(normals.GetOutput())
tex1.SetRFunction(planes)
#    tex1 FlipTextureOn
byuMapper = vtkDataSetMapper()
byuMapper.SetInput(tex1.GetOutput())
byuActor = vtkActor()
byuActor.SetMapper(byuMapper)
byuActor.SetTexture(texture)
byuActor.GetProperty().SetColor(cold_grey[0],cold_grey[1],cold_grey[2])

byu2 = vtkBYUReader()
byu2.SetGeometryFileName(VTK_DATA + "/motor.g")
byu2.SetPartNumber(2)
normals2 = vtkPolyDataNormals()
normals2.SetInput(byu2.GetOutput())
tex2 = vtkImplicitTextureCoords()
tex2.SetInput(normals2.GetOutput())
tex2.SetRFunction(planes)
#    tex2 FlipTextureOn
byuMapper2 = vtkDataSetMapper()
byuMapper2.SetInput(tex2.GetOutput())
byuActor2 = vtkActor()
byuActor2.SetMapper(byuMapper2)
byuActor2.SetTexture(texture)
byuActor2.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])

byu3 = vtkBYUReader()
byu3.SetGeometryFileName(VTK_DATA + "/motor.g")
byu3.SetPartNumber(3)

triangle3 = vtkTriangleFilter()
triangle3.SetInput(byu3.GetOutput())

normals3 = vtkPolyDataNormals()
normals3.SetInput(triangle3.GetOutput())
tex3 = vtkImplicitTextureCoords()
tex3.SetInput(normals3.GetOutput())
tex3.SetRFunction(planes)
#    tex3 FlipTextureOn
byuMapper3 = vtkDataSetMapper()
byuMapper3.SetInput(tex3.GetOutput())
byuActor3 = vtkActor()
byuActor3.SetMapper(byuMapper3)
byuActor3.SetTexture(texture)
byuActor3.GetProperty().SetColor(raw_sienna[0],raw_sienna[1],raw_sienna[2])

byu4 = vtkBYUReader()
byu4.SetGeometryFileName(VTK_DATA + "/motor.g")
byu4.SetPartNumber(4)
normals4 = vtkPolyDataNormals()
normals4.SetInput(byu4.GetOutput())
tex4 = vtkImplicitTextureCoords()
tex4.SetInput(normals4.GetOutput())
tex4.SetRFunction(planes)
#    tex4 FlipTextureOn
byuMapper4 = vtkDataSetMapper()
byuMapper4.SetInput(tex4.GetOutput())
byuActor4 = vtkActor()
byuActor4.SetMapper(byuMapper4)
byuActor4.SetTexture(texture)
byuActor4.GetProperty().SetColor(banana[0],banana[1],banana[2])

byu5 = vtkBYUReader()
byu5.SetGeometryFileName(VTK_DATA + "/motor.g")
byu5.SetPartNumber(5)
normals5 = vtkPolyDataNormals()
normals5.SetInput(byu5.GetOutput())
tex5 = vtkImplicitTextureCoords()
tex5.SetInput(normals5.GetOutput())
tex5.SetRFunction(planes)
#    tex5 FlipTextureOn
byuMapper5 = vtkDataSetMapper()
byuMapper5.SetInput(tex5.GetOutput())
byuActor5 = vtkActor()
byuActor5.SetMapper(byuMapper5)
byuActor5.SetTexture(texture)
byuActor5.GetProperty().SetColor(peach_puff[0],peach_puff[1],peach_puff[2])

# Add the actors to the renderer, set the background and size
#
ren.AddActor(byuActor)
ren.AddActor(byuActor2)
ren.AddActor(byuActor3)
byuActor3.VisibilityOff()
ren.AddActor(byuActor4)
ren.AddActor(byuActor5)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

camera = vtkCamera()
camera.SetFocalPoint(0.0286334,0.0362996,0.0379685)
camera.SetPosition(1.37067,1.08629,-1.30349)
camera.SetViewAngle(17.673)
camera.SetClippingRange(1,10)
camera.SetViewUp(-0.376306,-0.5085,-0.774482)

ren.SetActiveCamera(camera)

# render the image
iren.Initialize()
renWin.SetFileName("motor.ppm")

iren.Start()
