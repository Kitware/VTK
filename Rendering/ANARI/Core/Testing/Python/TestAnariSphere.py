# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
"""
This test verifies that actor level materials work with the ANARI back-end from Python
Mirrors TestAnariSphere.cxx
"""

import sys

from vtkmodules.vtkCommonCore import vtkDoubleArray
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkTestingRendering import vtkTesting

import vtkmodules.vtkRenderingAnariCore # to create vtkAnariRenderWindow


useDebugDevice = '--trace' in sys.argv

# set up the environment
renWin = vtkRenderWindow()
renderer = vtkRenderer()
renWin.AddRenderer(renderer)
renWin.SetSize(700, 700)

if renWin.GetClassName() != 'vtkAnariRenderWindow':
    raise RuntimeError(f"Expected vtkAnariRenderWindow but got {renWin.GetClassName()}")

anariDevice = renWin.GetAnariDevice()
anariRenderer = renWin.GetAnariRenderer()

anariDevice.SetupAnariDeviceFromLibrary('environment', 'default', useDebugDevice)

if useDebugDevice:
    testing = vtkTesting()
    traceDir = testing.GetTempDirectory()
    traceDir += '/anari-trace/TestAnariSphere'
    anariDevice.SetAnariDebugConfig(traceDir, 'code')

renWin.SetUseDebugDevice(useDebugDevice)

# General renderer parameters:
anariDevice.SetParameterf('ambientRadiance', 1.0)

# VisRTX specific renderer parameters:
anariRenderer.SetParameterf('lightFalloff', 0.5)
anariRenderer.SetParameterb('denoise', True)
anariRenderer.SetParameteri('pixelSamples', 8)

# make some predictable data to test with
# anything will do, but should have normals and textures coordinates
# for materials to work with
sphere = vtkSphereSource()
sphere.SetRadius(5)
sphere.SetPhiResolution(100)
sphere.SetThetaResolution(100)

# measure it so we can automate positioning
sphere.Update()
bds = sphere.GetOutput().GetBounds()
xo = bds[0]
xr = bds[1] - bds[0]
yo = bds[2]
zo = bds[4]
zr = bds[1] - bds[0]

# now what we actually want to test.
# draw the data at different places
# varying the visual characteristics each time

# plain old color
i = 0
j = 0
actor1 = vtkActor()
actor1.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
prop = actor1.GetProperty()
prop.SetMaterialName('matte')
prop.SetColor(1.0, 0.0, 0.0)  # Red
mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(sphere.GetOutputPort())
actor1.SetMapper(mapper1)
renderer.AddActor(actor1)

# color mapping
j += 1
actor2 = vtkActor()
actor2.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
mapper2 = vtkPolyDataMapper()
copy1 = vtkPolyData()
copy1.ShallowCopy(sphere.GetOutput())
mapper2.SetInputData(copy1)
da1 = vtkDoubleArray()
da1.SetNumberOfComponents(0)
da1.SetName('test_array')
for c in range(copy1.GetNumberOfPoints()):
    da1.InsertNextValue(c / float(copy1.GetNumberOfPoints()))
copy1.GetPointData().SetScalars(da1)
actor2.SetMapper(mapper2)
renderer.AddActor(actor2)

j += 1
actor3 = vtkActor()
actor3.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
mapper3 = vtkPolyDataMapper()
copy2 = vtkPolyData()
copy2.ShallowCopy(sphere.GetOutput())
mapper3.SetInputData(copy2)
da2 = vtkDoubleArray()
da2.SetNumberOfComponents(0)
da2.SetName('test_array')
for c in range(copy2.GetNumberOfCells()):
    da2.InsertNextValue(c / float(copy2.GetNumberOfCells()))
copy2.GetCellData().SetScalars(da2)
actor3.SetMapper(mapper3)
renderer.AddActor(actor3)

# invalid material, should warn but draw matte material
i = 1
j = 0
actor4 = vtkActor()
actor4.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
prop = actor4.GetProperty()
prop.SetMaterialName('flubber')
prop.SetColor(0.0, 0.0, 0.5)  # Navy
mapper4 = vtkPolyDataMapper()
mapper4.SetInputConnection(sphere.GetOutputPort())
actor4.SetMapper(mapper4)
renderer.AddActor(actor4)

# matte
j += 1
actor5 = vtkActor()
actor5.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
prop = actor5.GetProperty()
prop.SetMaterialName('matte')
prop.SetColor(0.0, 0.5, 0.0)  # Green
mapper5 = vtkPolyDataMapper()
mapper5.SetInputConnection(sphere.GetOutputPort())
actor5.SetMapper(mapper5)
renderer.AddActor(actor5)

# transparent matte
j += 1
actor6 = vtkActor()
actor6.SetPosition(xo + xr * 1.15 * i, yo, zo + zr * 1.1 * j)
prop = actor6.GetProperty()
prop.SetMaterialName('transparentMatte')
prop.SetOpacity(0.5)
prop.SetColor(0.5, 0.0, 0.5)  # Purple
mapper6 = vtkPolyDataMapper()
mapper6.SetInputConnection(sphere.GetOutputPort())
actor6.SetMapper(mapper6)
renderer.AddActor(actor6)

renderer.ResetCamera()
renderer.GetActiveCamera().Elevation(30)
renWin.Render()
