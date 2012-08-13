#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
byuReader = vtk.vtkBYUReader()
byuReader.SetGeometryFileName("" + str(VTK_DATA_ROOT) + "/Data/teapot.g")
byuMapper = vtk.vtkPolyDataMapper()
byuMapper.SetInputConnection(byuReader.GetOutputPort())
i = 0
while i < 9:
    locals()[get_variable_name("byuActor", i, "")] = vtk.vtkActor()
    locals()[get_variable_name("byuActor", i, "")].SetMapper(byuMapper)
    ren1.AddActor(locals()[get_variable_name("byuActor", i, "")])
    locals()[get_variable_name("hull", i, "")] = vtk.vtkHull()
    locals()[get_variable_name("hull", i, "")].SetInputConnection(byuReader.GetOutputPort())
    locals()[get_variable_name("hullMapper", i, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("hullMapper", i, "")].SetInputConnection(locals()[get_variable_name("hull", i, "")].GetOutputPort())
    locals()[get_variable_name("hullActor", i, "")] = vtk.vtkActor()
    locals()[get_variable_name("hullActor", i, "")].SetMapper(locals()[get_variable_name("hullMapper", i, "")])
    locals()[get_variable_name("hullActor", i, "")].GetProperty().SetColor(1,0,0)
    locals()[get_variable_name("hullActor", i, "")].GetProperty().SetAmbient(0.2)
    locals()[get_variable_name("hullActor", i, "")].GetProperty().SetDiffuse(0.8)
    locals()[get_variable_name("hullActor", i, "")].GetProperty().SetRepresentationToWireframe()
    ren1.AddActor(locals()[get_variable_name("hullActor", i, "")])
    i = i + 1

byuReader.Update()
diagonal = byuActor0.GetLength()
i = 0
j = -1
while j < 2:
    k = -1
    while k < 2:
        locals()[get_variable_name("byuActor", i, "")].AddPosition(expr.expr(globals(), locals(),["k","*","diagonal"]),expr.expr(globals(), locals(),["j","*","diagonal"]),0)
        locals()[get_variable_name("hullActor", i, "")].AddPosition(expr.expr(globals(), locals(),["k","*","diagonal"]),expr.expr(globals(), locals(),["j","*","diagonal"]),0)
        i = i + 1
        k = k + 1

    j = j + 1

hull0.AddCubeFacePlanes()
hull1.AddCubeEdgePlanes()
hull2.AddCubeVertexPlanes()
hull3.AddCubeFacePlanes()
hull3.AddCubeEdgePlanes()
hull3.AddCubeVertexPlanes()
hull4.AddRecursiveSpherePlanes(0)
hull5.AddRecursiveSpherePlanes(1)
hull6.AddRecursiveSpherePlanes(2)
hull7.AddRecursiveSpherePlanes(3)
hull8.AddRecursiveSpherePlanes(4)
# Add the actors to the renderer, set the background and size
#
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
iren.Initialize()
renWin.Render()
renWin.Render()
# prevent the tk window from showing up then start the event loop
# for testing
threshold = 15
# --- end of script --
