#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# All Plot3D scalar functions
#
# Create the RenderWindow, Renderer and both Actors
#
renWin = vtk.vtkRenderWindow()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
scalarLabels = "Density Pressure Temperature Enthalpy Internal_Energy Kinetic_Energy Velocity_Magnitude Stagnation_Energy Entropy Swirl"
scalarFunctions = "100 110 120 130 140 144 153 163 170 184"
camera = vtk.vtkCamera()
light = vtk.vtkLight()
math = vtk.vtkMath()
# All text actors will share the same text prop
textProp = vtk.vtkTextProperty()
textProp.SetFontSize(10)
textProp.SetFontFamilyToArial()
textProp.SetColor(0,0,0)
i = 0
for scalarFunction in scalarFunctions.split():
    locals()[get_variable_name("pl3d", scalarFunction, "")] = vtk.vtkMultiBlockPLOT3DReader()
    locals()[get_variable_name("pl3d", scalarFunction, "")].SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/bluntfinxyz.bin")
    locals()[get_variable_name("pl3d", scalarFunction, "")].SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/bluntfinq.bin")
    locals()[get_variable_name("pl3d", scalarFunction, "")].SetScalarFunctionNumber(expr.expr(globals(), locals(),["int","(","scalarFunction",")"]))
    locals()[get_variable_name("pl3d", scalarFunction, "")].Update()
    output = locals()[get_variable_name("pl3d", scalarFunction, "")].GetOutput().GetBlock(0)
    locals()[get_variable_name("plane", scalarFunction, "")] = vtk.vtkStructuredGridGeometryFilter()
    locals()[get_variable_name("plane", scalarFunction, "")].SetInputData(output)
    locals()[get_variable_name("plane", scalarFunction, "")].SetExtent(25,25,0,100,0,100)
    locals()[get_variable_name("mapper", scalarFunction, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("mapper", scalarFunction, "")].SetInputConnection(locals()[get_variable_name("plane", scalarFunction, "")].GetOutputPort())
    locals()[get_variable_name("mapper", scalarFunction, "")].SetScalarRange(output.GetPointData().GetScalars().GetRange())
    locals()[get_variable_name("actor", scalarFunction, "")] = vtk.vtkActor()
    locals()[get_variable_name("actor", scalarFunction, "")].SetMapper(locals()[get_variable_name("mapper", scalarFunction, "")])
    locals()[get_variable_name("ren", scalarFunction, "")] = vtk.vtkRenderer()
    locals()[get_variable_name("ren", scalarFunction, "")].SetBackground(0,0,.5)
    locals()[get_variable_name("ren", scalarFunction, "")].SetActiveCamera(camera)
    locals()[get_variable_name("ren", scalarFunction, "")].AddLight(light)
    renWin.AddRenderer(locals()[get_variable_name("ren", scalarFunction, "")])
    locals()[get_variable_name("ren", scalarFunction, "")].SetBackground(math.Random(.5,1),math.Random(.5,1),math.Random(.5,1))
    locals()[get_variable_name("ren", scalarFunction, "")].AddActor(locals()[get_variable_name("actor", scalarFunction, "")])
    locals()[get_variable_name("textMapper", scalarFunction, "")] = vtk.vtkTextMapper()
    locals()[get_variable_name("textMapper", scalarFunction, "")].SetInput(lindex(scalarLabels,i))
    locals()[get_variable_name("textMapper", scalarFunction, "")].SetTextProperty(textProp)
    locals()[get_variable_name("text", scalarFunction, "")] = vtk.vtkActor2D()
    locals()[get_variable_name("text", scalarFunction, "")].SetMapper(locals()[get_variable_name("textMapper", scalarFunction, "")])
    locals()[get_variable_name("text", scalarFunction, "")].SetPosition(2,3)
    if (info.command(globals(), locals(),  "rtExMath") == ""):
        locals()[get_variable_name("ren", scalarFunction, "")].AddActor2D(locals()[get_variable_name("text", scalarFunction, "")])
        pass
    i = i + 1

    pass
#
# now layout renderers
column = 1
row = 1
deltaX = expr.expr(globals(), locals(),["1.0","/","5.0"])
deltaY = expr.expr(globals(), locals(),["1.0","/","2.0"])
for scalarFunction in scalarFunctions.split():
    locals()[get_variable_name("ren", scalarFunction, "")].SetViewport(expr.expr(globals(), locals(),["(","column","-","1",")","*","deltaX"]),expr.expr(globals(), locals(),["(","row","-","1",")","*","deltaY"]),expr.expr(globals(), locals(),["column","*","deltaX"]),expr.expr(globals(), locals(),["row","*","deltaY"]))
    column = column + 1
    if (column > 5):
        column = 1
        row = row + 1
        pass

    pass
camera.SetViewUp(0,1,0)
camera.SetFocalPoint(0,0,0)
camera.SetPosition(1,0,0)
ren100.ResetCamera()
camera.Dolly(1.25)
ren100.ResetCameraClippingRange()
ren110.ResetCameraClippingRange()
ren120.ResetCameraClippingRange()
ren130.ResetCameraClippingRange()
ren140.ResetCameraClippingRange()
ren144.ResetCameraClippingRange()
ren153.ResetCameraClippingRange()
ren163.ResetCameraClippingRange()
ren170.ResetCameraClippingRange()
ren184.ResetCameraClippingRange()
light.SetPosition(camera.GetPosition())
light.SetFocalPoint(camera.GetFocalPoint())
renWin.SetSize(600,180)
renWin.Render()
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
