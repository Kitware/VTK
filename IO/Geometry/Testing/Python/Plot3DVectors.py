#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

#
# All Plot3D vector functions
#
# Create the RenderWindow, Renderer and both Actors
#
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
ren1 = vtk.vtkRenderer()
ren1.SetBackground(.8,.8,.2)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
vectorLabels = "Velocity Vorticity Momentum Pressure_Gradient"
vectorFunctions = "200 201 202 210"
camera = vtk.vtkCamera()
light = vtk.vtkLight()
# All text actors will share the same text prop
textProp = vtk.vtkTextProperty()
textProp.SetFontSize(10)
textProp.SetFontFamilyToArial()
textProp.SetColor(.3,1,1)
i = 0
for vectorFunction in vectorFunctions.split():
    locals()[get_variable_name("pl3d", vectorFunction, "")] = vtk.vtkMultiBlockPLOT3DReader()
    locals()[get_variable_name("pl3d", vectorFunction, "")].SetXYZFileName("" + str(VTK_DATA_ROOT) + "/Data/bluntfinxyz.bin")
    locals()[get_variable_name("pl3d", vectorFunction, "")].SetQFileName("" + str(VTK_DATA_ROOT) + "/Data/bluntfinq.bin")
    locals()[get_variable_name("pl3d", vectorFunction, "")].SetVectorFunctionNumber(expr.expr(globals(), locals(),["int","(","vectorFunction",")"]))
    locals()[get_variable_name("pl3d", vectorFunction, "")].Update()
    output = locals()[get_variable_name("pl3d", vectorFunction, "")].GetOutput().GetBlock(0)
    locals()[get_variable_name("plane", vectorFunction, "")] = vtk.vtkStructuredGridGeometryFilter()
    locals()[get_variable_name("plane", vectorFunction, "")].SetInputData(output)
    locals()[get_variable_name("plane", vectorFunction, "")].SetExtent(25,25,0,100,0,100)
    locals()[get_variable_name("hog", vectorFunction, "")] = vtk.vtkHedgeHog()
    locals()[get_variable_name("hog", vectorFunction, "")].SetInputConnection(locals()[get_variable_name("plane", vectorFunction, "")].GetOutputPort())
    maxnorm = output.GetPointData().GetVectors().GetMaxNorm()
    locals()[get_variable_name("hog", vectorFunction, "")].SetScaleFactor(expr.expr(globals(), locals(),["1.0","/","maxnorm"]))
    locals()[get_variable_name("mapper", vectorFunction, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("mapper", vectorFunction, "")].SetInputConnection(locals()[get_variable_name("hog", vectorFunction, "")].GetOutputPort())
    locals()[get_variable_name("actor", vectorFunction, "")] = vtk.vtkActor()
    locals()[get_variable_name("actor", vectorFunction, "")].SetMapper(locals()[get_variable_name("mapper", vectorFunction, "")])
    locals()[get_variable_name("ren", vectorFunction, "")] = vtk.vtkRenderer()
    locals()[get_variable_name("ren", vectorFunction, "")].SetBackground(0.5,.5,.5)
    locals()[get_variable_name("ren", vectorFunction, "")].SetActiveCamera(camera)
    locals()[get_variable_name("ren", vectorFunction, "")].AddLight(light)
    renWin.AddRenderer(locals()[get_variable_name("ren", vectorFunction, "")])
    locals()[get_variable_name("ren", vectorFunction, "")].AddActor(locals()[get_variable_name("actor", vectorFunction, "")])
    locals()[get_variable_name("textMapper", vectorFunction, "")] = vtk.vtkTextMapper()
    locals()[get_variable_name("textMapper", vectorFunction, "")].SetInput(lindex(vectorLabels,i))
    locals()[get_variable_name("textMapper", vectorFunction, "")].SetTextProperty(textProp)
    locals()[get_variable_name("text", vectorFunction, "")] = vtk.vtkActor2D()
    locals()[get_variable_name("text", vectorFunction, "")].SetMapper(locals()[get_variable_name("textMapper", vectorFunction, "")])
    locals()[get_variable_name("text", vectorFunction, "")].SetPosition(2,5)
    if (info.command(globals(), locals(),  "rtExMath") == ""):
        locals()[get_variable_name("ren", vectorFunction, "")].AddActor2D(locals()[get_variable_name("text", vectorFunction, "")])
        pass
    i = i + 1

    pass
#
# now layout renderers
column = 1
row = 1
deltaX = expr.expr(globals(), locals(),["1.0","/","2.0"])
deltaY = expr.expr(globals(), locals(),["1.0","/","2.0"])
for vectorFunction in vectorFunctions.split():
    locals()[get_variable_name("ren", vectorFunction, "")].SetViewport(expr.expr(globals(), locals(),["(","column","-","1",")","*","deltaX","+","(","deltaX","*",".05",")"]),expr.expr(globals(), locals(),["(","row","-","1",")","*","deltaY","+","(","deltaY","*",".05",")"]),expr.expr(globals(), locals(),["column","*","deltaX","-","(","deltaX","*",".05",")"]),expr.expr(globals(), locals(),["row","*","deltaY","-","(","deltaY","*",".05",")"]))
    column = column + 1
    if (column > 2):
        column = 1
        row = row + 1
        pass

    pass
camera.SetViewUp(1,0,0)
camera.SetFocalPoint(0,0,0)
camera.SetPosition(.4,-.5,-.75)
ren200.ResetCamera()
camera.Dolly(1.25)
ren200.ResetCameraClippingRange()
ren201.ResetCameraClippingRange()
ren202.ResetCameraClippingRange()
ren210.ResetCameraClippingRange()
light.SetPosition(camera.GetPosition())
light.SetFocalPoint(camera.GetFocalPoint())
renWin.SetSize(350,350)
renWin.Render()
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
