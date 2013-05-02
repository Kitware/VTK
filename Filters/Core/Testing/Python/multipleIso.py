#!/usr/bin/env python
import math
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

range = output.GetPointData().GetScalars().GetRange()
min = range[0]
max = range[1]
value = (min + max) / 2.0

cf = vtk.vtkContourFilter()
cf.SetInputData(output)
cf.SetValue(0, value)
cf.UseScalarTreeOn()

numberOfContours = 5
epsilon = float(max - min) / float(numberOfContours * 10)
min = min + epsilon
max = max - epsilon
i = 1
while i <= numberOfContours:
    cf.SetValue(0, min + ((i - 1) / float(numberOfContours - 1)) * (max - min))
    cf.Update()
    idx = str(i)
    exec("pd" + idx + " = vtk.vtkPolyData()")
    eval("pd" + idx).CopyStructure(cf.GetOutput())
    eval("pd" + idx).GetPointData().DeepCopy(cf.GetOutput().GetPointData())

    exec("mapper" + idx + " = vtk.vtkPolyDataMapper()")
    eval("mapper" + idx).SetInputData(eval("pd" + idx))
    eval("mapper" + idx).SetScalarRange(
      output.GetPointData().GetScalars().GetRange())

    exec("actor" + idx + " = vtk.vtkActor()")
    eval("actor" + idx).AddPosition(0, i * 12, 0)
    eval("actor" + idx).SetMapper(eval("mapper" + idx))

    ren1.AddActor(eval("actor" + idx))

    i += 1

# Add the actors to the renderer, set the background and size
#
ren1.SetBackground(.3, .3, .3)

renWin.SetSize(450, 150)

# cam1 = ren1.GetActiveCamera()
ren1.GetActiveCamera().SetPosition(-36.3762, 32.3855, 51.3652)
ren1.GetActiveCamera().SetFocalPoint(8.255, 33.3861, 29.7687)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0, 0, 1)
ren1.ResetCameraClippingRange()

iren.Initialize()
#iren.Start()
