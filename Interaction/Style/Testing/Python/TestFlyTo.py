#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkLODActor
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

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a sphere source and actor
#
sphere = vtkSphereSource()

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkLODActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetDiffuseColor(GetRGBColor('banana'))
sphereActor.GetProperty().SetSpecular(.4)
sphereActor.GetProperty().SetSpecularPower(20)

# create the spikes using a cone source and the sphere source
#
cone = vtkConeSource()
cone.SetResolution(20)

glyph = vtkGlyph3D()
glyph.SetInputConnection(sphere.GetOutputPort())
glyph.SetSourceConnection(cone.GetOutputPort())
glyph.SetVectorModeToUseNormal()
glyph.SetScaleModeToScaleByVector()
glyph.SetScaleFactor(0.25)

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInputConnection(glyph.GetOutputPort())

spikeActor = vtkLODActor()
spikeActor.SetMapper(spikeMapper)
spikeActor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
spikeActor.GetProperty().SetSpecular(.4)
spikeActor.GetProperty().SetSpecularPower(20)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(sphereActor)
ren1.AddActor(spikeActor)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(300, 300)

# render the image
#
ren1.ResetCamera()

cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.4)
cam1.Azimuth(30)
cam1.Elevation(30)
renWin.Render()

iren.FlyTo(ren1, 0.37723, 0.154699, 0.204326)

#iren.Start()
