#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


ren = vtkRenderer()

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)



reader = vtkSLCReader()
reader.SetFileName(VTK_DATA + "/vw_knee.slc")

reader.Update()

white_tfun = vtkPiecewiseFunction()
white_tfun.AddPoint(0,1.0)
white_tfun.AddPoint(255,1.0)


tfun = vtkPiecewiseFunction()
tfun.AddPoint(70,0.0)
tfun.AddPoint(80,1.0)

ren.SetBackground(.1,.2,.4)

vol_prop = vtkVolumeProperty()
#vol_prop.SetColor(white_tfun[0],white_tfun[1],white_tfun[2])
vol_prop.SetColor(white_tfun)
vol_prop.SetScalarOpacity(tfun)
vol_prop.SetInterpolationTypeToLinear()
vol_prop.ShadeOn()

comp_func = vtkVolumeRayCastCompositeFunction()

volmap = vtkVolumeRayCastMapper()
volmap.SetVolumeRayCastFunction(comp_func)
volmap.SetInput(reader.GetOutput())
volmap.SetSampleDistance(1.0)

vol = vtkVolume()
vol.SetProperty(vol_prop)
vol.SetMapper(volmap)

ren.AddVolume(vol)

contour = vtkContourFilter()
contour.SetInput(reader.GetOutput())
contour.SetValue(0,30.0)

points = vtkPoints()
points.InsertPoint(0,100.0,150.0,130.0)
points.InsertPoint(1,100.0,150.0,130.0)
points.InsertPoint(2,100.0,150.0,130.0)

normals = vtkNormals()
normals.InsertNormal(0,1.0,0.0,0.0)
normals.InsertNormal(1,0.0,1.0,0.0)
normals.InsertNormal(2,0.0,0.0,1.0)
  

planes = vtkPlanes()
planes.SetPoints(points)
planes.SetNormals(normals)
  

clipper = vtkClipPolyData()
clipper.SetInput(contour.GetOutput())
clipper.SetClipFunction(planes)
clipper.GenerateClipScalarsOn()

skin_mapper = vtkPolyDataMapper()
skin_mapper.SetInput(clipper.GetOutput())
skin_mapper.ScalarVisibilityOff()

skin = vtkActor()
skin.SetMapper(skin_mapper)
skin.GetProperty().SetColor(0.8,0.4,0.2)

ren.AddActor(skin)

renWin.SetSize(200,200)


ren.GetActiveCamera().SetPosition(-47.5305,-319.315,92.0083)
ren.GetActiveCamera().SetFocalPoint(78.9121,89.8372,95.1229)
ren.GetActiveCamera().SetViewUp(-0.00708891,0.00980254,-0.999927)
ren.GetActiveCamera().SetClippingRange(42.8255,2141.28)

iren.Initialize()

renWin.Render()


iren.Start()
