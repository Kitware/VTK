#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
renderer = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(renderer)
renWin.StereoCapableWindowOn()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
reader = vtk.vtkGenericEnSightReader()
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/viga.case")

geometry = vtk.vtkGeometryFilter()
geometry.SetInputConnection(reader.GetOutputPort())

mapper = vtk.vtkCompositePolyDataMapper2()
mapper.SetInputConnection(geometry.GetOutputPort())

actor = vtk.vtkActor()
actor.SetMapper(mapper)

# render
renderer.AddActor(actor)
renWin.Render()
