#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKPatentedPython import *


# Create ren1dering stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# ingest data file
reader = vtkPolyDataReader()
reader.SetFileName(VTK_DATA + "/vtk.vtk")

# create implicit model of vtk
imp = vtkImplicitModeller()
imp.SetInput(reader.GetOutput())
imp.SetSampleDimensions(50,50,40)
imp.SetMaximumDistance(0.25)
imp.SetAdjustDistance(0.5)

# create swept surface
transforms = vtkTransformCollection()
t1 = vtkTransform()
t1.Identity()
t2 = vtkTransform()
t2.Translate(0,0,2.5)
t2.RotateZ(90.0)
transforms.AddItem(t1)
transforms.AddItem(t2)

sweptSurfaceFilter = vtkSweptSurface()
sweptSurfaceFilter.SetInput(imp.GetOutput())
sweptSurfaceFilter.SetTransforms(transforms)
sweptSurfaceFilter.SetSampleDimensions(100,70,40)
sweptSurfaceFilter.SetNumberOfInterpolationSteps(20)

iso = vtkContourFilter()
iso.SetInput(sweptSurfaceFilter.GetOutput())
iso.SetValue(0,0.33)

sweptSurfaceMapper = vtkPolyDataMapper()
sweptSurfaceMapper.SetInput(iso.GetOutput())
sweptSurfaceMapper.ScalarVisibilityOff()

sweptSurface = vtkActor()
sweptSurface.SetMapper(sweptSurfaceMapper)
sweptSurface.GetProperty().SetColor(0.2510,0.8784,0.8157)

ren.AddActor(sweptSurface)
ren.SetBackground(1,1,1)

renWin.SetSize(500,500)

ren.GetActiveCamera().Zoom(1.5)
iren.Initialize()





iren.Start()
