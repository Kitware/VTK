#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

cone = vtk.vtkConeSource()
reflect = vtk.vtkReflectionFilter()
reflect.SetInputConnection(cone.GetOutputPort())
reflect.SetPlaneToXMax()
reflect2 = vtk.vtkReflectionFilter()
reflect2.SetInputConnection(reflect.GetOutputPort())
reflect2.SetPlaneToYMax()
reflect3 = vtk.vtkReflectionFilter()
reflect3.SetInputConnection(reflect2.GetOutputPort())
reflect3.SetPlaneToZMax()
mapper = vtk.vtkDataSetMapper()
mapper.SetInputConnection(reflect3.GetOutputPort())
actor = vtk.vtkActor()
actor.SetMapper(mapper)
ren1 = vtk.vtkRenderer()
ren1.AddActor(actor)
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.SetSize(200,200)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
renWin.Render()
# --- end of script --
