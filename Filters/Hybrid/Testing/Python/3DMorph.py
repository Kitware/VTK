#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# use implicit modeller / interpolation to perform 3D morphing
#
# make the letter v, t and k
letterV = vtk.vtkVectorText()
letterV.SetText("v")
letterT = vtk.vtkVectorText()
letterT.SetText("t")
letterK = vtk.vtkVectorText()
letterK.SetText("k")

# create implicit models of each
blobbyV = vtk.vtkImplicitModeller()
blobbyV.SetInputConnection(letterV.GetOutputPort())
blobbyV.SetMaximumDistance(.2)
blobbyV.SetSampleDimensions(50, 50, 12)
blobbyV.SetModelBounds(-0.5, 1.5, -0.5, 1.5, -0.5, 0.5)

blobbyT = vtk.vtkImplicitModeller()
blobbyT.SetInputConnection(letterT.GetOutputPort())
blobbyT.SetMaximumDistance(.2)
blobbyT.SetSampleDimensions(50, 50, 12)
blobbyT.SetModelBounds(-0.5, 1.5, -0.5, 1.5, -0.5, 0.5)

blobbyK = vtk.vtkImplicitModeller()
blobbyK.SetInputConnection(letterK.GetOutputPort())
blobbyK.SetMaximumDistance(.2)
blobbyK.SetSampleDimensions(50, 50, 12)
blobbyK.SetModelBounds(-0.5, 1.5, -0.5, 1.5, -0.5, 0.5)

# Interpolate the data
interpolate = vtk.vtkInterpolateDataSetAttributes()
interpolate.AddInputConnection(blobbyV.GetOutputPort())
interpolate.AddInputConnection(blobbyT.GetOutputPort())
interpolate.AddInputConnection(blobbyK.GetOutputPort())
interpolate.SetT(0.0)

# extract an iso surface
blobbyIso = vtk.vtkContourFilter()
blobbyIso.SetInputConnection(interpolate.GetOutputPort())
blobbyIso.SetValue(0, 0.1)

# map to rendering primitives
blobbyMapper = vtk.vtkPolyDataMapper()
blobbyMapper.SetInputConnection(blobbyIso.GetOutputPort())
blobbyMapper.ScalarVisibilityOff()
# now an actor
blobby = vtk.vtkActor()
blobby.SetMapper(blobbyMapper)
blobby.GetProperty().SetDiffuseColor(GetRGBColor('banana'))

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

camera = vtk.vtkCamera()
camera.SetClippingRange(0.265, 13.2)
camera.SetFocalPoint(0.539, 0.47464, 0)
camera.SetPosition(0.539, 0.474674, 2.644)
camera.SetViewUp(0, 1, 0)

ren1.SetActiveCamera(camera)

#  now  make a renderer and tell it about lights and actors

renWin.SetSize(300, 350)

ren1.AddActor(blobby)
ren1.SetBackground(1, 1, 1)
renWin.Render()

subIters = 4.0
i = 0
while i < 2:
    j = 1
    while j <= subIters:
        t = i + j / subIters
        interpolate.SetT(t)
        renWin.Render()
        j += 1
    i += 1

renWin.Render()

iren.Initialize()
#iren.Start()
