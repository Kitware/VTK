#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKContribPython import *

#catch  load vtktcl 
# Convert a 3d Studio file to Renderman RIB

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

importer.GetRenderer().SetBackground(0.1,0.2,0.4)
importer.GetRenderWindow().SetSize(300,300)

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

#
# export to rib format
exporter = vtkRIBExporter()
exporter.SetFilePrefix(importExport)
exporter.SetRenderWindow(importer.GetRenderWindow())
exporter.BackgroundOn()
exporter.Write()

exit()
iren.Start()
