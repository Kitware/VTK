#!/usr/bin/env python

# This simple example shows how to do basic rendering and pipeline
# creation. It also demonstrates the use of the LODActor.

# Import the VTK-Python module
import vtk
from vtk.util.misc import vtkGetDataRoot
from vtk.util.colors import *
# Get the location of the data.
VTK_DATA_ROOT = vtkGetDataRoot()

# This creates a polygonal cylinder model with eight circumferential
# facets.
part = vtk.vtkSTLReader()
part.SetFileName(VTK_DATA_ROOT + "/Data/42400-IDGH.stl")

# The mapper is responsible for pushing the geometry into the graphics
# library. It may also do color mapping, if scalars or other
# attributes are defined.
partMapper = vtk.vtkPolyDataMapper()
partMapper.SetInput(part.GetOutput())

# The LOD actor is a special type of actor. It will change appearance
# in order to render faster. At the highest resolution, it renders
# ewverything just like an actor. The middle level is a point cloud,
# and the lowest level is a simple bounding box.
partActor = vtk.vtkLODActor()
partActor.SetMapper(partMapper)
partActor.GetProperty().SetColor(light_grey)
partActor.RotateX(30.0)
partActor.RotateY(-45.0)

# Create the graphics structure. The renderer renders into the render
# window. The render window interactor captures mouse events and will
# perform appropriate camera or actor manipulation depending on the
# nature of the events.
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(partActor)
ren.SetBackground(0.1, 0.2, 0.4)
renWin.SetSize(200, 200)

# We'll zoom in a little by accessing the camera and invoking a "Zoom"
# method on it.
ren.GetActiveCamera().Zoom(1.5)

# This starts the event loop.
iren.Initialize()
renWin.Render()
iren.Start()
