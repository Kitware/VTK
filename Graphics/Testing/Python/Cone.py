#!/usr/bin/env python

import sys

for i in range(0, len(sys.argv)):
    if sys.argv[i] == '-A' and i < len(sys.argv)-1:
        sys.path = sys.path + [sys.argv[i+1]]

from vtkpython import *

# create a rendering window and renderer
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(300,300)

# create an actor and give it cone geometry
cone = vtkConeSource()
cone.SetResolution(8)
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# assign our actor to the renderer
ren.AddActor(coneActor)
renWin.Render()
retVal = vtkRegressionTestImage(renWin)

sys.exit( not retVal )
