#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

renWin = vtk.vtkRenderWindow()
renWin.OffScreenRenderingOn()
renWin.SetMultiSamples(0)
ren = vtk.vtkRenderer()
renWin.AddRenderer(ren)
cone = vtk.vtkConeSource()
mp = vtk.vtkPolyDataMapper()
mp.SetInputConnection(cone.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mp)
ren.AddActor(actor)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
