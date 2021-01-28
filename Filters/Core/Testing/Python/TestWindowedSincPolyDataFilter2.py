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

# Test the NormalizeCoordinates option (as compared to
# TestWindowedSincPolyDataFilter.py). The sphere is placed
# away from the origin, and scaled in size.

# Control resolution of the test
res = 100

# Set up rendering
ren0 = vtk.vtkRenderer()
ren0.SetViewport(0,0,0.5,1.0)
ren1 = vtk.vtkRenderer()
ren1.SetViewport(0.5,0,1.0,1.0)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.SetActiveCamera(ren0.GetActiveCamera())

# Pipeline stuff. Create a sphere source at high resolution.
sphere = vtk.vtkSphereSource()
sphere.SetCenter(100,200,400)
sphere.SetRadius(100)
sphere.SetThetaResolution(res)
sphere.SetPhiResolution(int(res/2))

# Add some noise
random = vtk.vtkBrownianPoints()
random.SetInputConnection(sphere.GetOutputPort())
random.SetMinimumSpeed(0.0)
random.SetMaximumSpeed(1)

# Now warp the sphere with the noise
warp = vtk.vtkWarpVector()
warp.SetInputConnection(random.GetOutputPort())
warp.SetScaleFactor(1.0)

# Smooth it
smooth = vtk.vtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(warp.GetOutputPort())
smooth.SetNumberOfIterations(20)
smooth.FeatureEdgeSmoothingOff()
smooth.BoundarySmoothingOff()
smooth.NonManifoldSmoothingOff()
smooth.SetPassBand(0.1)
smooth.GenerateErrorScalarsOn()
smooth.GenerateErrorVectorsOn()
smooth.NormalizeCoordinatesOn()
smooth.Update()

mapper0 = vtk.vtkPolyDataMapper()
mapper0.SetInputConnection(warp.GetOutputPort())

actor0 = vtk.vtkActor()
actor0.SetMapper(mapper0)
actor0.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor0.GetProperty().SetDiffuse(.8)
actor0.GetProperty().SetSpecular(.4)
actor0.GetProperty().SetSpecularPower(30)

mapper1 = vtk.vtkPolyDataMapper()
mapper1.SetInputConnection(smooth.GetOutputPort())
mapper1.SetScalarRange(smooth.GetOutput().GetScalarRange())

actor1 = vtk.vtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor1.GetProperty().SetDiffuse(.8)
actor1.GetProperty().SetSpecular(.4)
actor1.GetProperty().SetSpecularPower(30)

# Add the actors to the renderer, set the background and size
#
ren0.AddActor(actor0)
ren0.SetBackground(1, 1, 1)
ren1.AddActor(actor1)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(450, 300)
ren0.ResetCamera()
renWin.Render()
ren0.GetActiveCamera().Zoom(1.25)

iren.Initialize()
iren.Start()
