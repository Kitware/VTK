#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a vtk file
#
byu = vtkBYUReader()
byu.SetGeometryFileName(VTK_DATA + "/fohe.g")
normals = vtkPolyDataNormals()
normals.SetInput(byu.GetOutput())
byuMapper = vtkPolyDataMapper()
byuMapper.SetInput(normals.GetOutput())
byuActor = vtkActor()
byuActor.SetMapper(byuMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(byuActor)
ren.SetBackground(0.2,0.3,0.4)
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()
iren.Start()
