#!/usr/bin/env python

# In this example we show the use of the
# vtkBandedPolyDataContourFilter.  This filter creates separate,
# constant colored bands for a range of scalar values. Each band is
# bounded by two scalar values, and the cell data lying within the
# value has the same cell scalar value.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


# The lookup table is similar to that used by maps. Two hues are used:
# a brown for land, and a blue for water. The value of the hue is
# changed to give the effect of elevation.
Scale = 5
lutWater = vtk.vtkLookupTable()
lutWater.SetNumberOfColors(10)
lutWater.SetHueRange(0.58, 0.58)
lutWater.SetSaturationRange(0.5, 0.1)
lutWater.SetValueRange(0.5, 1.0)
lutWater.Build()
lutLand = vtk.vtkLookupTable()
lutLand.SetNumberOfColors(10)
lutLand.SetHueRange(0.1, 0.1)
lutLand.SetSaturationRange(0.4, 0.1)
lutLand.SetValueRange(0.55, 0.9)
lutLand.Build()


# The DEM reader reads data and creates an output image.
demModel = vtk.vtkDEMReader()
demModel.SetFileName(VTK_DATA_ROOT + "/Data/SainteHelens.dem")
demModel.Update()

# We shrink the terrain data down a bit to yield better performance for 
# this example.
shrinkFactor = 4
shrink = vtk.vtkImageShrink3D()
shrink.SetShrinkFactors(shrinkFactor, shrinkFactor, 1)
shrink.SetInput(demModel.GetOutput())
shrink.AveragingOn()

# Convert the image into polygons.
geom = vtk.vtkImageDataGeometryFilter()
geom.SetInput(shrink.GetOutput())

# Warp the polygons based on elevation.
warp = vtk.vtkWarpScalar()
warp.SetInput(geom.GetOutput())
warp.SetNormal(0, 0, 1)
warp.UseNormalOn()
warp.SetScaleFactor(Scale)

# Create the contour bands.
bcf = vtk.vtkBandedPolyDataContourFilter()
bcf.SetInput(warp.GetPolyDataOutput())
bcf.GenerateValues(15, demModel.GetOutput().GetScalarRange())
bcf.SetScalarModeToIndex()
bcf.GenerateContourEdgesOn()

# Compute normals to give a better look.
normals = vtk.vtkPolyDataNormals()
normals.SetInput(bcf.GetOutput())
normals.SetFeatureAngle(60)
normals.ConsistencyOff()
normals.SplittingOff()

demMapper = vtk.vtkPolyDataMapper()
demMapper.SetInput(normals.GetOutput())
demMapper.SetScalarRange(0, 10)
demMapper.SetLookupTable(lutLand)
demMapper.SetScalarModeToUseCellData()

demActor = vtk.vtkLODActor()
demActor.SetMapper(demMapper)

## Create contour edges
edgeMapper = vtk.vtkPolyDataMapper()
edgeMapper.SetInput(bcf.GetContourEdgesOutput())
edgeMapper.SetResolveCoincidentTopologyToPolygonOffset()
edgeActor = vtk.vtkActor()
edgeActor.SetMapper(edgeMapper)
edgeActor.GetProperty().SetColor(0, 0, 0)

## Test clipping
# Create the contour bands.
bcf2 = vtk.vtkBandedPolyDataContourFilter()
bcf2.SetInput(warp.GetPolyDataOutput())
bcf2.ClippingOn()
bcf2.GenerateValues(10, 1000, 2000)
bcf2.SetScalarModeToValue()

# Compute normals to give a better look.
normals2 = vtk.vtkPolyDataNormals()
normals2.SetInput(bcf2.GetOutput())
normals2.SetFeatureAngle(60)
normals2.ConsistencyOff()
normals2.SplittingOff()

lut = vtk.vtkLookupTable()
lut.SetNumberOfColors(10)
demMapper2 = vtk.vtkPolyDataMapper()
demMapper2.SetInput(normals2.GetOutput())
demMapper2.SetScalarRange(demModel.GetOutput().GetScalarRange())
demMapper2.SetLookupTable(lut)
demMapper2.SetScalarModeToUseCellData()

demActor2 = vtk.vtkLODActor()
demActor2.SetMapper(demMapper2)
demActor2.AddPosition(0, 15000, 0)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(demActor)
ren.AddActor(demActor2)
ren.AddActor(edgeActor)

ren.SetBackground(.4, .4, .4)
renWin.SetSize(375, 200)

cam = vtk.vtkCamera()
cam.SetPosition(-17438.8, 2410.62, 25470.8)
cam.SetFocalPoint(3985.35, 11930.6, 5922.14)
cam.SetViewUp(0, 0, 1)
ren.SetActiveCamera(cam)
ren.ResetCameraClippingRange()

iren.Initialize()
iren.SetDesiredUpdateRate(1)

def CheckAbort(obj, event):
    foo = renWin.GetEventPending()
    if foo != 0:
        renWin.SetAbortRender(1)

renWin.AddObserver("AbortCheckEvent", CheckAbort)
renWin.Render()

renWin.Render()
iren.Start()
