#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400,400)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

reader = vtkSTLReader()
reader.SetFileName("../../../vtkdata/cadPart.stl")

cpd = vtkCleanPolyData()
cpd.SetInput(reader.GetOutput())

normals = vtkPolyDataNormals()
normals.SetFeatureAngle(30)
normals.SetInput(cpd.GetOutput())

conn = vtkPolyDataConnectivityFilter()
conn.SetMaxRecursionDepth(1000)
conn.SetInput(normals.GetOutput())
conn.ColorRegionsOn()
conn.SetExtractionModeToAllRegions()
conn.Update()

mapper = vtkPolyDataMapper()
mapper.SetInput(conn.GetOutput())
mapper.SetScalarRange(conn.GetOutput().GetScalarRange())

actor = vtkActor()
actor.SetMapper(mapper)

ren.AddActor(actor)
ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(60)
ren.GetActiveCamera().Dolly(1.2)

#wm withdraw .

iren.Initialize()

#renWin SetFileName polyConnColorRegions.tcl.ppm
#renWin SaveImageAsPPM

iren.Start()
