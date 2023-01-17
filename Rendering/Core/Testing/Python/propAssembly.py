#!/usr/bin/env python
from vtkmodules.vtkFiltersSources import (
    vtkConeSource,
    vtkCubeSource,
    vtkCylinderSource,
    vtkSphereSource,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkAssembly,
    vtkPolyDataMapper,
    vtkPropAssembly,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# demonstrates the use of vtkPropAssembly
# create four parts: a top level assembly and three primitives
#
sphere = vtkSphereSource()
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())
sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.SetOrigin(2,1,3)
sphereActor.RotateY(6)
sphereActor.SetPosition(2.25,0,0)
sphereActor.GetProperty().SetColor(1,0,1)
cube = vtkCubeSource()
cubeMapper = vtkPolyDataMapper()
cubeMapper.SetInputConnection(cube.GetOutputPort())
cubeActor = vtkActor()
cubeActor.SetMapper(cubeMapper)
cubeActor.SetPosition(0.0,.25,0)
cubeActor.GetProperty().SetColor(0,0,1)
cone = vtkConeSource()
coneMapper = vtkPolyDataMapper()
coneMapper.SetInputConnection(cone.GetOutputPort())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.SetPosition(0,0,.25)
coneActor.GetProperty().SetColor(0,1,0)
cylinder = vtkCylinderSource()
#top part
cylinderMapper = vtkPolyDataMapper()
cylinderMapper.SetInputConnection(cylinder.GetOutputPort())
cylinderActor = vtkActor()
cylinderActor.SetMapper(cylinderMapper)
cylinderActor.GetProperty().SetColor(1,0,0)
compositeAssembly = vtkAssembly()
compositeAssembly.AddPart(cylinderActor)
compositeAssembly.AddPart(sphereActor)
compositeAssembly.AddPart(cubeActor)
compositeAssembly.AddPart(coneActor)
compositeAssembly.SetOrigin(5,10,15)
compositeAssembly.AddPosition(5,0,0)
compositeAssembly.RotateX(15)
# Build the prop assembly out of a vtkActor and a vtkAssembly
assembly = vtkPropAssembly()
assembly.AddPart(compositeAssembly)
assembly.AddPart(coneActor)
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
#
ren1.AddViewProp(assembly)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)
# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()
# should create the same image as assembly.tcl
# prevent the tk window from showing up then start the event loop
# --- end of script --
