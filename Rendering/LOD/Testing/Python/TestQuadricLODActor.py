#!/usr/bin/env python
import vtk
from vtk.test import Testing
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

# Test the quadric decimation LOD actor
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# pipeline stuff
#
sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(150)
sphere.SetThetaResolution(150)

plane = vtk.vtkPlaneSource()
plane.SetXResolution(150)
plane.SetYResolution(150)

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(sphere.GetOutputPort())
mapper.SetInputConnection(plane.GetOutputPort())

actor = vtk.vtkQuadricLODActor()
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
