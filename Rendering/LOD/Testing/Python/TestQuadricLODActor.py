#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkFiltersSources import (
    vtkPlaneSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingLOD import vtkQuadricLODActor
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

# Test the quadric decimation LOD actor
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = vtkSphereSource()
sphere.SetPhiResolution(150)
sphere.SetThetaResolution(150)

plane = vtkPlaneSource()
plane.SetXResolution(150)
plane.SetYResolution(150)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetInputConnection(plane.GetOutputPort())

actor = vtkQuadricLODActor()
actor.SetMapper(mapper)
actor.DeferLODConstructionOff()
actor.GetProperty().SetRepresentationToWireframe()
actor.GetProperty().SetDiffuseColor(GetRGBColor('tomato'))
actor.GetProperty().SetDiffuse(.8)
actor.GetProperty().SetSpecular(.4)
actor.GetProperty().SetSpecularPower(30)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)

iren.Initialize()
#iren.Start()
