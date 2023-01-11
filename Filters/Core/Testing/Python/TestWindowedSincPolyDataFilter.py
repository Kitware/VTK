#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkWindowedSincPolyDataFilter
from vtkmodules.vtkFiltersGeneral import (
    vtkBrownianPoints,
    vtkWarpVector,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# Control resolution of the test
res = 100

# Set up rendering
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1.0)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1.0,1.0)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.SetActiveCamera(ren0.GetActiveCamera())

# Pipeline stuff. Create a sphere source at high resolution.
sphere = vtkSphereSource()
sphere.SetThetaResolution(res)
sphere.SetPhiResolution(int(res/2))

# Add some noise
random = vtkBrownianPoints()
random.SetInputConnection(sphere.GetOutputPort())
random.SetMinimumSpeed(0.0)
random.SetMaximumSpeed(0.01)

# Now warp the sphere with the noise
warp = vtkWarpVector()
warp.SetInputConnection(random.GetOutputPort())
warp.SetScaleFactor(1.0)

# Smooth it
smooth = vtkWindowedSincPolyDataFilter()
smooth.SetInputConnection(warp.GetOutputPort())
smooth.SetNumberOfIterations(20)
smooth.FeatureEdgeSmoothingOff()
smooth.BoundarySmoothingOff()
smooth.NonManifoldSmoothingOff()
smooth.SetPassBand(0.1)
smooth.GenerateErrorScalarsOn()
smooth.GenerateErrorVectorsOn()
smooth.Update()

mapper0 = vtkPolyDataMapper()
mapper0.SetInputConnection(warp.GetOutputPort())

actor0 = vtkActor()
actor0.SetMapper(mapper0)
actor0.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor0.GetProperty().SetDiffuse(.8)
actor0.GetProperty().SetSpecular(.4)
actor0.GetProperty().SetSpecularPower(30)

mapper1 = vtkPolyDataMapper()
mapper1.SetInputConnection(smooth.GetOutputPort())
mapper1.SetScalarRange(smooth.GetOutput().GetScalarRange())

actor1 = vtkActor()
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
