#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

cone = vtk.vtkConeSource()
cone.SetRadius(0.05)
cone.SetHeight(0.25)
cone.SetResolution(256)
cone.SetCenter(0.15,0.0,0.15)
rotate = vtk.vtkRotationFilter()
rotate.SetInputConnection(cone.GetOutputPort())
rotate.SetAxisToZ()
rotate.SetCenter(0.0,0.0,0.0)
rotate.SetAngle(45)
rotate.SetNumberOfCopies(7)
rotate.CopyInputOn()
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(rotate.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1 = vtk.vtkRenderer()
ren1.AddActor(actor)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(512,512)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
# --- end of script --
