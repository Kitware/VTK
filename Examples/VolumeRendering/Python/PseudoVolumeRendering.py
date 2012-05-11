#!/usr/bin/env python

# Perform psuedo volume rendering in a structured grid by compositing
# translucent cut planes. This same trick can be used for unstructured
# grids. Note that for better results, more planes can be created. Also,
# if your data is vtkImageData, there are much faster methods for volume
# rendering.

import vtk
from vtk.util.colors import *
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create pipeline. Read structured grid data.
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

pl3d_output = pl3d.GetOutput().GetBlock(0)

# A convenience, use this filter to limit data for experimentation.
extract = vtk.vtkExtractGrid()
extract.SetVOI(1, 55, -1000, 1000, -1000, 1000)
extract.SetInputData(pl3d_output)

# The (implicit) plane is used to do the cutting
plane = vtk.vtkPlane()
plane.SetOrigin(0, 4, 2)
plane.SetNormal(0, 1, 0)

# The cutter is set up to process each contour value over all cells
# (SetSortByToSortByCell). This results in an ordered output of polygons
# which is key to the compositing.
cutter = vtk.vtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)
cutter.GenerateCutScalarsOff()
cutter.SetSortByToSortByCell()

clut = vtk.vtkLookupTable()
clut.SetHueRange(0, .67)
clut.Build()

cutterMapper = vtk.vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.SetScalarRange(.18, .7)
cutterMapper.SetLookupTable(clut)

cut = vtk.vtkActor()
cut.SetMapper(cutterMapper)

# Add in some surface geometry for interest.
iso = vtk.vtkContourFilter()
iso.SetInputData(pl3d_output)
iso.SetValue(0, .22)
normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)
isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOff()
isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetDiffuseColor(tomato)
isoActor.GetProperty().SetSpecularColor(white)
isoActor.GetProperty().SetDiffuse(.8)
isoActor.GetProperty().SetSpecular(.5)
isoActor.GetProperty().SetSpecularPower(30)

outline = vtk.vtkStructuredGridOutlineFilter()
outline.SetInputData(pl3d_output)
outlineTubes = vtk.vtkTubeFilter()
outlineTubes.SetInputConnection(outline.GetOutputPort())
outlineTubes.SetRadius(.1)

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outlineTubes.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create the RenderWindow, Renderer and Interactor
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(outlineActor)
outlineActor.GetProperty().SetColor(banana)
ren.AddActor(isoActor)
isoActor.VisibilityOn()
ren.AddActor(cut)
opacity = .1
cut.GetProperty().SetOpacity(1)
ren.SetBackground(1, 1, 1)
renWin.SetSize(640, 480)

cam1 = ren.GetActiveCamera()
cam1.SetClippingRange(3.95297, 50)
cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
cam1.SetPosition(2.7439, -37.3196, 38.7167)
cam1.ComputeViewPlaneNormal()
cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

# Cut: generates n cut planes normal to camera's view plane
def Cut(n):
    global cam1, opacity
    plane.SetNormal(cam1.GetViewPlaneNormal())
    plane.SetOrigin(cam1.GetFocalPoint())
    cutter.GenerateValues(n, -5, 5)
    clut.SetAlphaRange(opacity, opacity)
    renWin.Render()


# Generate 10 cut planes
Cut(20)

iren.Initialize()
renWin.Render()
iren.Start()
