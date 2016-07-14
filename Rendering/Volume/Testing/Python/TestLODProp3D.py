#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple volume rendering example.
reader = vtk.vtkSLCReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/sphere.slc")
reader.Update()
# Create transfer functions for opacity and color
opacityTransferFunction = vtk.vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(20,0.0)
opacityTransferFunction.AddPoint(255,1.0)
colorTransferFunction = vtk.vtkColorTransferFunction()
colorTransferFunction.AddHSVPoint(0.0,0.01,1.0,1.0)
colorTransferFunction.AddHSVPoint(127.5,0.50,1.0,1.0)
colorTransferFunction.AddHSVPoint(255.0,0.99,1.0,1.0)
colorTransferFunction.SetColorSpaceToHSV()
# Create properties, mappers, volume actors, and ray cast function
volumeProperty = vtk.vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.SetInterpolationTypeToLinear()
volumeMapper = vtk.vtkFixedPointVolumeRayCastMapper()
volumeMapper.SetInputConnection(reader.GetOutputPort())
sphereSource = vtk.vtkSphereSource()
sphereSource.SetCenter(25,25,25)
sphereSource.SetRadius(30)
sphereSource.SetThetaResolution(15)
sphereSource.SetPhiResolution(15)
geoMapper = vtk.vtkPolyDataMapper()
geoMapper.SetInputConnection(sphereSource.GetOutputPort())
lod = vtk.vtkLODProp3D()
geoID = lod.AddLOD(geoMapper,0.0)
volID = lod.AddLOD(volumeMapper,volumeProperty,0.0)
property = vtk.vtkProperty()
property.SetColor(1,0,0)
lod.SetLODProperty(geoID,property)
# Okay now the graphics stuff
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(256,256)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
ren1.GetCullers().InitTraversal()
culler = ren1.GetCullers().GetNextItem()
culler.SetSortingStyleToBackToFront()
ren1.AddViewProp(lod)
# render a few times
renWin.Render()
renWin.Render()
renWin.Render()
# disable the geometry and render
lod.DisableLOD(geoID)
renWin.Render()
# disable the volume and render
lod.EnableLOD(geoID)
lod.DisableLOD(volID)
renWin.Render()
# chose the geometry to render
lod.EnableLOD(volID)
lod.AutomaticLODSelectionOff()
lod.SetSelectedLODID(geoID)
renWin.Render()
# choose the volume
lod.SetSelectedLODID(volID)
renWin.Render()
# this should be the volID - remove it
id = lod.GetLastRenderedLODID()
lod.RemoveLOD(id)
lod.AutomaticLODSelectionOn()
renWin.Render()
iren.Initialize()
# --- end of script --
