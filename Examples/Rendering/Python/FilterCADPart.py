#!/usr/bin/env python

# This simple example shows how to do simple filtering in a pipeline.
# See CADPart.py and Cylinder.py for related information.

import vtk
from vtk.util.colors import light_grey
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This creates a polygonal cylinder model with eight circumferential
# facets.
part = vtk.vtkSTLReader()
part.SetFileName(VTK_DATA_ROOT + "/Data/42400-IDGH.stl")

# A filter is a module that takes at least one input and produces at
# least one output. The SetInput and GetOutput methods are used to do
# the connection. What is returned by GetOutput is a particulat
# dataset type. If the type is compatible with the SetInput method,
# then the filters can be connected together.
#
# Here we add a filter that computes surface normals from the geometry.
shrink = vtk.vtkShrinkPolyData()
shrink.SetInputConnection(part.GetOutputPort())
shrink.SetShrinkFactor(0.85)

# The mapper is responsible for pushing the geometry into the graphics
# library. It may also do color mapping, if scalars or other
# attributes are defined.
partMapper = vtk.vtkPolyDataMapper()
partMapper.SetInputConnection(shrink.GetOutputPort())

# The LOD actor is a special type of actor. It will change appearance
# in order to render faster. At the highest resolution, it renders
# ewverything just like an actor. The middle level is a point cloud,
# and the lowest level is a simple bounding box.
partActor = vtk.vtkLODActor()
partActor.SetMapper(partMapper)
partActor.GetProperty().SetColor(light_grey)
partActor.RotateX(30.0)
partActor.RotateY(-45.0)

# Create the graphics structure. The renderer renders into the
# render window. The render window interactor captures mouse events
# and will perform appropriate camera or actor manipulation
# depending on the nature of the events.
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
ren.ResetCamera()
ren.GetActiveCamera().Zoom(1.5)

iren.Initialize()
renWin.Render()
# Start the event loop.
iren.Start()
