#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This example demonstrates the use of the linear extrusion filter and
# the USA state outline vtk dataset. It also tests the triangulation filter.
# get the interactor ui
# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline - read data
#
reader = vtk.vtkPolyDataReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/usa.vtk")
# okay, now create some extrusion filters with actors for each US state
math = vtk.vtkMath()
i = 0
while i < 51:
    locals()[get_variable_name("extractCell", i, "")] = vtk.vtkGeometryFilter()
    locals()[get_variable_name("extractCell", i, "")].SetInputConnection(reader.GetOutputPort())
    locals()[get_variable_name("extractCell", i, "")].CellClippingOn()
    locals()[get_variable_name("extractCell", i, "")].SetCellMinimum(i)
    locals()[get_variable_name("extractCell", i, "")].SetCellMaximum(i)
    locals()[get_variable_name("tf", i, "")] = vtk.vtkTriangleFilter()
    locals()[get_variable_name("tf", i, "")].SetInputConnection(locals()[get_variable_name("extractCell", i, "")].GetOutputPort())
    locals()[get_variable_name("extrude", i, "")] = vtk.vtkLinearExtrusionFilter()
    locals()[get_variable_name("extrude", i, "")].SetInputConnection(locals()[get_variable_name("tf", i, "")].GetOutputPort())
    locals()[get_variable_name("extrude", i, "")].SetExtrusionType(1)
    locals()[get_variable_name("extrude", i, "")].SetVector(0,0,1)
    locals()[get_variable_name("extrude", i, "")].CappingOn()
    locals()[get_variable_name("extrude", i, "")].SetScaleFactor(math.Random(1,10))
    locals()[get_variable_name("mapper", i, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("mapper", i, "")].SetInputConnection(locals()[get_variable_name("extrude", i, "")].GetOutputPort())
    locals()[get_variable_name("actor", i, "")] = vtk.vtkActor()
    locals()[get_variable_name("actor", i, "")].SetMapper(locals()[get_variable_name("mapper", i, "")])
    locals()[get_variable_name("actor", i, "")].GetProperty().SetColor(math.Random(0,1),math.Random(0,1),math.Random(0,1))
    ren1.AddActor(locals()[get_variable_name("actor", i, "")])
    i = i + 1

ren1.SetBackground(1,1,1)
renWin.SetSize(500,250)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(10.2299,511.497)
cam1.SetPosition(-119.669,-25.5502,79.0198)
cam1.SetFocalPoint(-115.96,41.6709,1.99546)
cam1.SetViewUp(-0.0013035,0.753456,0.657497)
# render the image
#
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
