#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Extract a surface net from a labeled image and color it by the
# BoundaryLabels cell data. This exercises the cell-data color mapping on a
# triangulated surface net that contains degenerate triangles, which must be
# kept so that per-cell colors stay aligned with the rendered primitives.

# Read the labeled image
reader = vtk.vtkXMLImageDataReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/label_map.vti")

# Extract the surface net
snets = vtk.vtkSurfaceNets3D()
snets.SetInputConnection(reader.GetOutputPort())

timer = vtk.vtkTimerLog()
timer.StartTimer()
snets.Update()
timer.StopTimer()
print("Time to generate Surface Net: {0}".format(timer.GetElapsedTime()))

surface = snets.GetOutput()
boundaryLabels = surface.GetCellData().GetArray("BoundaryLabels")

# Color by the first component of the BoundaryLabels cell data
mapper = vtk.vtkPolyDataMapper()
mapper.SetInputData(surface)
mapper.SetScalarModeToUseCellData()
mapper.SelectColorArray("BoundaryLabels")
mapper.SetArrayComponent(0)
mapper.SetColorModeToMapScalars()
mapper.ScalarVisibilityOn()
mapper.SetScalarRange(boundaryLabels.GetRange(0))

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# Create the RenderWindow, Renderer and Actor
ren = vtk.vtkRenderer()
ren.AddActor(actor)
ren.SetBackground(0.2, 0.3, 0.4)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(400, 300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Camera
camera = ren.GetActiveCamera()
bounds = surface.GetBounds()
center = (
    0.5 * (bounds[0] + bounds[1]),
    0.5 * (bounds[2] + bounds[3]),
    0.5 * (bounds[4] + bounds[5]),
)
camera.SetFocalPoint(*center)
camera.SetPosition(center[0], center[1] + 1, center[2])
camera.SetViewUp(0, 0, 1)
ren.ResetCamera()
camera.Zoom(1.3)
ren.ResetCameraClippingRange()

renWin.Render()
iren.Start()
