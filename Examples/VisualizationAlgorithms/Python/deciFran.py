#!/usr/bin/env python

# This example shows how to use decimation to reduce a polygonal
# mesh. We also use mesh smoothing and generate surface normals to
# give a pleasing result.

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# We start by reading some data that was originally captured from a
# Cyberware laser digitizing system.
fran = vtk.vtkPolyDataReader()
fran.SetFileName(VTK_DATA_ROOT + "/Data/fran_cut.vtk")

# We want to preserve topology (not let any cracks form). This may
# limit the total reduction possible, which we have specified at 90%.
deci = vtk.vtkDecimatePro()
deci.SetInput(fran.GetOutput())
deci.SetTargetReduction(0.9)
deci.PreserveTopologyOn()
normals = vtk.vtkPolyDataNormals()
normals.SetInput(fran.GetOutput())
normals.FlipNormalsOn()
franMapper = vtk.vtkPolyDataMapper()
franMapper.SetInput(normals.GetOutput())
franActor = vtk.vtkActor()
franActor.SetMapper(franMapper)
franActor.GetProperty().SetColor(1.0, 0.49, 0.25)

# Create the RenderWindow, Renderer and both Actors
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(franActor)
ren.SetBackground(1, 1, 1)
renWin.SetSize(250, 250)

cam1 = vtk.vtkCamera()
cam1.SetClippingRange(0.0475572, 2.37786)
cam1.SetFocalPoint(0.052665, -0.129454, -0.0573973)
cam1.SetPosition(0.327637, -0.116299, -0.256418)
cam1.SetViewUp(-0.0225386, 0.999137, 0.034901)
ren.SetActiveCamera(cam1)

iren.Initialize()
renWin.Render()
iren.Start()
