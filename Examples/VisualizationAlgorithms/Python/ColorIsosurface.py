#!/usr/bin/env python

# This example shows how to color an isosurface with other
# data. Basically an isosurface is generated, and a data array is
# selected and used by the mapper to color the surface.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Read some data. The important thing here is to read a function as a
# data array as well as the scalar and vector.  (here function 153 is
# named "Velocity Magnitude").Later this data array will be used to
# color the isosurface.
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.AddFunction(153)
pl3d.Update()
pl3d.DebugOn()
pl3d_output = pl3d.GetOutput().GetBlock(0)

# The contour filter uses the labeled scalar (function number 100
# above to generate the contour surface; all other data is
# interpolated during the contouring process.
iso = vtk.vtkContourFilter()
iso.SetInputData(pl3d_output)
iso.SetValue(0, .24)

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)

# We indicate to the mapper to use the velcoity magnitude, which is a
# vtkDataArray that makes up part of the point attribute data.
isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOn()
isoMapper.SetScalarRange(0, 1500)
isoMapper.SetScalarModeToUsePointFieldData()
isoMapper.ColorByArrayComponent("VelocityMagnitude", 0)

isoActor = vtk.vtkLODActor()
isoActor.SetMapper(isoMapper)
isoActor.SetNumberOfCloudPoints(1000)

outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the usual rendering stuff.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(500, 500)
ren.SetBackground(0.1, 0.2, 0.4)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

iren.Initialize()
renWin.Render()
iren.Start()
