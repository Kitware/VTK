#!/usr/bin/env python

# This example demonstrates the conversion of point data to cell data.
# The conversion is necessary because we want to threshold data based
# on cell scalar values.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read some data with point data attributes. The data is from a
# plastic blow molding process (e.g., to make plastic bottles) and
# consists of two logical components: a mold and a parison. The
# parison is the hot plastic that is being molded, and the mold is
# clamped around the parison to form its shape.
reader = vtk.vtkUnstructuredGridReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/blow.vtk")
reader.SetScalarsName("thickness9")
reader.SetVectorsName("displacement9")

# Convert the point data to cell data. The point data is passed
# through the filter so it can be warped. The vtkThresholdFilter then
# thresholds based on cell scalar values and extracts a portion of the
# parison whose cell scalar values lie between 0.25 and 0.75.
p2c = vtk.vtkPointDataToCellData()
p2c.SetInput(reader.GetOutput())
p2c.PassPointDataOn()
warp = vtk.vtkWarpVector()
warp.SetInput(p2c.GetUnstructuredGridOutput())
thresh = vtk.vtkThreshold()
thresh.SetInput(warp.GetOutput())
thresh.ThresholdBetween(0.25, 0.75)
thresh.SetAttributeModeToUseCellData()

# This is used to extract the mold from the parison. 
connect = vtk.vtkConnectivityFilter()
connect.SetInput(thresh.GetOutput())
connect.SetExtractionModeToSpecifiedRegions()
connect.AddSpecifiedRegion(0)
connect.AddSpecifiedRegion(1)
moldMapper = vtk.vtkDataSetMapper()
moldMapper.SetInput(reader.GetOutput())
moldMapper.ScalarVisibilityOff()
moldActor = vtk.vtkActor()
moldActor.SetMapper(moldMapper)
moldActor.GetProperty().SetColor(.2, .2, .2)
moldActor.GetProperty().SetRepresentationToWireframe()

# The threshold filter has been used to extract the parison.
connect2 = vtk.vtkConnectivityFilter()
connect2.SetInput(thresh.GetOutput())
parison = vtk.vtkGeometryFilter()
parison.SetInput(connect2.GetOutput())
normals2 = vtk.vtkPolyDataNormals()
normals2.SetInput(parison.GetOutput())
normals2.SetFeatureAngle(60)
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.0, 0.66667)
parisonMapper = vtk.vtkPolyDataMapper()
parisonMapper.SetInput(normals2.GetOutput())
parisonMapper.SetLookupTable(lut)
parisonMapper.SetScalarRange(0.12, 1.0)
parisonActor = vtk.vtkActor()
parisonActor.SetMapper(parisonMapper)

# We generate some contour lines on the parison.
cf = vtk.vtkContourFilter()
cf.SetInput(connect2.GetOutput())
cf.SetValue(0, .5)
contourMapper = vtk.vtkPolyDataMapper()
contourMapper.SetInput(cf.GetOutput())
contours = vtk.vtkActor()
contours.SetMapper(contourMapper)

# Create graphics stuff
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(moldActor)
ren.AddActor(parisonActor)
ren.AddActor(contours)

ren.GetActiveCamera().Azimuth(60)
ren.GetActiveCamera().Roll(-90)
ren.GetActiveCamera().Dolly(2)
ren.ResetCameraClippingRange()

ren.SetBackground(1, 1, 1)
renWin.SetSize(750, 400)

iren.Initialize()
renWin.Render()
iren.Start()
