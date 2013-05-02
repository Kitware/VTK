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
byuReader.SetGeometryFileName(VTK_DATA_ROOT + "/Data/teapot.g")

byuMapper = vtk.vtkPolyDataMapper()
byuMapper.SetInputConnection(byuReader.GetOutputPort())

i = 0
while i < 9:
    idx = str(i)
    exec("byuActor" + idx + " = vtk.vtkActor()")
    eval("byuActor" + idx).SetMapper(byuMapper)

    ren1.AddActor(eval("byuActor" + idx))

    exec("hull" + idx + " = vtk.vtkHull()")
    eval("hull" + idx).SetInputConnection(byuReader.GetOutputPort())

    exec("hullMapper" + idx + " = vtk.vtkPolyDataMapper()")
    eval("hullMapper" + idx).SetInputConnection(
      eval("hull" + idx).GetOutputPort())

    exec("hullActor" + idx + " = vtk.vtkActor()")
    eval("hullActor" + idx).SetMapper(eval("hullMapper" + idx))
    eval("hullActor" + idx).GetProperty().SetColor(1, 0, 0)
    eval("hullActor" + idx).GetProperty().SetAmbient(0.2)
    eval("hullActor" + idx).GetProperty().SetDiffuse(0.8)
    eval("hullActor" + idx).GetProperty().SetRepresentationToWireframe()

    ren1.AddActor(eval("hullActor" + idx))

    i += 1

byuReader.Update()

diagonal = byuActor0.GetLength()
i = 0
j = -1
while j < 2:
    k = -1
    while k < 2:
        idx = str(i)
        eval("byuActor" + idx).AddPosition(k * diagonal, j * diagonal, 0)
        eval("hullActor" + idx).AddPosition(k * diagonal, j * diagonal, 0)
        i += 1
        k += 1

    j += 1

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
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(500, 500)

iren.Initialize()
renWin.Render()

# for testing
threshold = 15

#iren.Start()
