#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# Demonstrates the 3D Studio Importer

# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

importer = vtk3DSImporter()
importer.SetRenderWindow(renWin)
importer.ComputeNormalsOn()
importer.SetFileName("../../../vtkdata/Viewpoint/iflamigm.3ds")
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
ren.GetActiveCamera().ComputeViewPlaneNormal()
ren.GetActiveCamera().SetViewUp(0,0,1)

#
# let the renderer compute good position and focal point
#
ren.ResetCamera()
ren.GetActiveCamera().Dolly(1.4)

# render the image
#
iren.Initialize()
#wm withdraw .

#renWin SetFileName "flamingo.tcl.ppm"
#renWin SaveImageAsPPM
iren.Start()
