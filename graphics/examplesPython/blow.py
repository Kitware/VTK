#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Create blow molding image (data point 5)

# create reader and warp data with vectors
reader = vtkUnstructuredGridReader()
reader.SetFileName(VTK_DATA + "/blow.vtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")
warp = vtkWarpVector()
warp.SetInput(reader.GetOutput())

# extract mold from mesh using connectivity
connect = vtkConnectivityFilter()
connect.SetInput(warp.GetOutput())
connect.SetExtractionModeToSpecifiedRegions()
connect.AddSpecifiedRegion(0)
connect.AddSpecifiedRegion(1)
moldMapper = vtkDataSetMapper()
moldMapper.SetInput(reader.GetOutput())
moldMapper.ScalarVisibilityOff()
moldActor = vtkActor()
moldActor.SetMapper(moldMapper)
moldActor.GetProperty().SetColor(.2,.2,.2)
moldActor.GetProperty().SetRepresentationToWireframe()

# extract parison from mesh using connectivity
connect2 = vtkConnectivityFilter()
connect2.SetInput(warp.GetOutput())
connect2.SetExtractionModeToSpecifiedRegions()
connect2.AddSpecifiedRegion(2)
parison = vtkGeometryFilter()
parison.SetInput(connect2.GetOutput())
normals2 = vtkPolyDataNormals()
normals2.SetInput(parison.GetOutput())
normals2.SetFeatureAngle(60)
lut = vtkLookupTable()
lut.SetHueRange(0.0,0.66667)
parisonMapper = vtkPolyDataMapper()
parisonMapper.SetInput(normals2.GetOutput())
parisonMapper.SetLookupTable(lut)
parisonMapper.SetScalarRange(0.12,1.0)
parisonActor = vtkActor()
parisonActor.SetMapper(parisonMapper)

cf = vtkContourFilter()
cf.SetInput(connect2.GetOutput())
cf.SetValue(0,.5)
contourMapper = vtkPolyDataMapper()
contourMapper.SetInput(cf.GetOutput())
contours = vtkActor()
contours.SetMapper(contourMapper)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(moldActor)
ren.AddActor(parisonActor)
ren.AddActor(contours)
ren.GetActiveCamera().Azimuth(60)
ren.GetActiveCamera().Roll(-90)
ren.GetActiveCamera().Dolly(2)
ren.ResetCameraClippingRange()
ren.SetBackground(1,1,1)
renWin.SetSize(750,400)

iren.Initialize()





iren.Start()
