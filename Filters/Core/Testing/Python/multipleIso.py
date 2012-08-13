#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# get the interactor ui
## Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/combxyz.bin")
pl3d.SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
range = output.GetPointData().GetScalars().GetRange()
min = lindex(range,0)
max = lindex(range,1)
value = expr.expr(globals(), locals(),["(","min","+","max",")","/","2.0"])
cf = vtk.vtkContourFilter()
cf.SetInputData(output)
cf.SetValue(0,value)
cf.UseScalarTreeOn()
numberOfContours = 5
epsilon = expr.expr(globals(), locals(),["double","(","max","-","min",")","/","double","(","numberOfContours","*","10",")"])
min = expr.expr(globals(), locals(),["min","+","epsilon"])
max = expr.expr(globals(), locals(),["max","-","epsilon"])
i = 1
while i <= numberOfContours:
    cf.SetValue(0,expr.expr(globals(), locals(),["min","+","((","i","-","1",")","/","double","(","numberOfContours","-","1",")",")*(","max","-","min",")"]))
    cf.Update()
    locals()[get_variable_name("pd", i, "")] = vtk.vtkPolyData()
    locals()[get_variable_name("pd", i, "")].CopyStructure(cf.GetOutput())
    locals()[get_variable_name("pd", i, "")].GetPointData().DeepCopy(cf.GetOutput().GetPointData())
    locals()[get_variable_name("mapper", i, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("mapper", i, "")].SetInputData(locals()[get_variable_name("pd", i, "")])
    locals()[get_variable_name("mapper", i, "")].SetScalarRange(output.GetPointData().GetScalars().GetRange())
    locals()[get_variable_name("actor", i, "")] = vtk.vtkActor()
    locals()[get_variable_name("actor", i, "")].AddPosition(0,expr.expr(globals(), locals(),["i","*","12"]),0)
    locals()[get_variable_name("actor", i, "")].SetMapper(locals()[get_variable_name("mapper", i, "")])
    ren1.AddActor(locals()[get_variable_name("actor", i, "")])
    i = i + 1

# Add the actors to the renderer, set the background and size
#
ren1.SetBackground(.3,.3,.3)
renWin.SetSize(450,150)
cam1 = ren1.GetActiveCamera()
ren1.GetActiveCamera().SetPosition(-36.3762,32.3855,51.3652)
ren1.GetActiveCamera().SetFocalPoint(8.255,33.3861,29.7687)
ren1.GetActiveCamera().SetViewAngle(30)
ren1.GetActiveCamera().SetViewUp(0,0,1)
ren1.ResetCameraClippingRange()
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
