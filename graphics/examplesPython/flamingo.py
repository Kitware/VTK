#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Demonstrates the 3D Studio Importer


ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

importer = vtk3DSImporter()
importer.SetRenderWindow(renWin)
importer.ComputeNormalsOn()
importer.SetFileName(VTK_DATA + "/Viewpoint/iflamigm.3ds")
importer.Read()

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

importer.GetRenderer().SetBackground(0.1,0.2,0.4)
importer.GetRenderWindow().SetSize(300,300)

#
# the importer created the renderer
renCollection=renWin.GetRenderers()
renCollection.InitTraversal()
ren=renCollection.GetNextItem()
#
# change view up to +z
#
ren.GetActiveCamera().SetPosition(0,1,0)
ren.GetActiveCamera().SetFocalPoint(0,0,0)
ren.GetActiveCamera().SetViewUp(0,0,1)

#
# let the renderer compute good position and focal point
#
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.4)
ren.ResetCameraClippingRange()

# render the image
#
iren.Initialize()

iren.Start()
