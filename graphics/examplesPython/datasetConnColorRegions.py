#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(256,256)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

reader = vtkSTLReader()
reader.SetFileName(VTK_DATA + "/cadPart.stl")

cpd = vtkCleanPolyData()
cpd.SetInput(reader.GetOutput())

normals = vtkPolyDataNormals()
normals.SetFeatureAngle(30)
normals.SetInput(cpd.GetOutput())

conn = vtkConnectivityFilter()
conn.SetMaxRecursionDepth(1000)
conn.SetInput(normals.GetOutput())
conn.ColorRegionsOn()
conn.SetExtractionModeToAllRegions()
conn.Update()

#
# we need an explicit geometry filter to turn merging off
# if we just used a dataset mapper, the points are merged and
# the normals are not what we expect
#

geometry = vtkGeometryFilter()
geometry.SetInput(conn.GetOutput())
geometry.MergingOff()

mapper = vtkPolyDataMapper()
mapper.SetInput(geometry.GetOutput())
mapper.SetScalarRange(conn.GetOutput().GetScalarRange())

actor = vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)
ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(60)
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()

renWin.Render()
renWin.SetFileName("datasetConnColorRegions.ppm")
renWin.SaveImageAsPPM()

iren.Initialize()
iren.Start()
