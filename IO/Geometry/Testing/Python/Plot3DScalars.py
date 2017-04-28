#!/usr/bin/env python
import vtk
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
scalarLabels = ["Density", "Pressure", "Temperature", "Enthalpy",
                 "Internal_Energy", "Kinetic_Energy", "Velocity_Magnitude",
                  "Stagnation_Energy", "Entropy", "Swirl"]
scalarFunctions = ["100", "110", "120", "130",
                    "140", "144", "153",
                    "163", "170", "184"]

camera = vtk.vtkCamera()

light = vtk.vtkLight()

math = vtk.vtkMath()

# All text actors will share the same text prop
textProp = vtk.vtkTextProperty()
textProp.SetFontSize(10)
textProp.SetFontFamilyToArial()
textProp.SetColor(0, 0, 0)

i = 0
for scalarFunction in scalarFunctions:
    exec("pl3d" + scalarFunction + " = vtk.vtkMultiBlockPLOT3DReader()")
    eval("pl3d" + scalarFunction).SetXYZFileName(
      VTK_DATA_ROOT + "/Data/bluntfinxyz.bin")
    eval("pl3d" + scalarFunction).SetQFileName(
      VTK_DATA_ROOT + "/Data/bluntfinq.bin")
    eval("pl3d" + scalarFunction).SetScalarFunctionNumber(int(scalarFunction))
    eval("pl3d" + scalarFunction).Update()

    output = eval("pl3d" + scalarFunction).GetOutput().GetBlock(0)

    exec("plane" + scalarFunction + " = vtk.vtkStructuredGridGeometryFilter()")
    eval("plane" + scalarFunction).SetInputData(output)
    eval("plane" + scalarFunction).SetExtent(25, 25, 0, 100, 0, 100)

    exec("mapper" + scalarFunction + " = vtk.vtkPolyDataMapper()")
    eval("mapper" + scalarFunction).SetInputConnection(
      eval("plane" + scalarFunction).GetOutputPort())
    eval("mapper" + scalarFunction).SetScalarRange(
      output.GetPointData().GetScalars().GetRange())

    exec("actor" + scalarFunction + " = vtk.vtkActor()")
    eval("actor" + scalarFunction).SetMapper(eval("mapper" + scalarFunction))

    exec("ren" + scalarFunction + " = vtk.vtkRenderer()")
    eval("ren" + scalarFunction).SetBackground(0, 0, .5)
    eval("ren" + scalarFunction).SetActiveCamera(camera)
    eval("ren" + scalarFunction).AddLight(light)

    renWin.AddRenderer(eval("ren" + scalarFunction))

    eval("ren" + scalarFunction).SetBackground(
      math.Random(.5, 1), math.Random(.5, 1), math.Random(.5, 1))
    eval("ren" + scalarFunction).AddActor(eval("actor" + scalarFunction))

    exec("textMapper" + scalarFunction + " = vtk.vtkTextMapper()")
    eval("textMapper" + scalarFunction).SetInput(scalarLabels[i])
    eval("textMapper" + scalarFunction).SetTextProperty(textProp)

#    exec("text" + scalarFunction + " = vtk.vtkActor2D()")
#    eval("text" + scalarFunction).SetMapper(eval("textMapper" + scalarFunction))
#    eval("text" + scalarFunction).SetPosition(2, 3)
#
#    eval("ren" + scalarFunction).AddActor2D(eval("text" + scalarFunction))

    i += 1
#
# now layout the renderers
column = 1
row = 1
deltaX = 1.0 / 5.0
deltaY = 1.0 / 2.0
for scalarFunction in scalarFunctions:
    eval("ren" + scalarFunction).SetViewport(
      (column - 1) * deltaX, (row - 1) * deltaY, column * deltaX, row * deltaY)
    column += 1
    if (column > 5):
        column = 1
        row += 1

camera.SetViewUp(0, 1, 0)
camera.SetFocalPoint(0, 0, 0)
camera.SetPosition(1, 0, 0)

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

renWin.SetSize(600, 180)
renWin.Render()

# render the image
#
iren.Initialize()
# iren.Start()
