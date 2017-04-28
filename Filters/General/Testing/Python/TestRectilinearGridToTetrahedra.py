#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

# SetUp the pipeline
FormMesh = vtk.vtkRectilinearGridToTetrahedra()
FormMesh.SetInput(4, 2, 2, 1, 1, 1, 0.001)
FormMesh.RememberVoxelIdOn()

TetraEdges = vtk.vtkExtractEdges()
TetraEdges.SetInputConnection(FormMesh.GetOutputPort())

tubes = vtk.vtkTubeFilter()
tubes.SetInputConnection(TetraEdges.GetOutputPort())
tubes.SetRadius(0.05)
tubes.SetNumberOfSides(6)

# Run the pipeline 3 times, with different conversions to TetMesh
FormMesh.SetTetraPerCellTo5()

tubes.Update()

Tubes1 = vtk.vtkPolyData()
Tubes1.DeepCopy(tubes.GetOutput())

FormMesh.SetTetraPerCellTo6()

tubes.Update()

Tubes2 = vtk.vtkPolyData()
Tubes2.DeepCopy(tubes.GetOutput())

FormMesh.SetTetraPerCellTo12()

tubes.Update()

Tubes3 = vtk.vtkPolyData()
Tubes3.DeepCopy(tubes.GetOutput())

# Run the pipeline once more, this time converting some cells to
# 5 and some data to 12 TetMesh
# Determine which cells are which
DivTypes = vtk.vtkIntArray()
numCell = FormMesh.GetInput().GetNumberOfCells()
DivTypes.SetNumberOfValues(numCell)

i = 0
while i < numCell:
    DivTypes.SetValue(i, 5 + (7 * (i % 4)))
    i += 1

# Finish this pipeline
FormMesh.SetTetraPerCellTo5And12()
FormMesh.GetInput().GetCellData().SetScalars(DivTypes)

tubes.Update()

Tubes4 = vtk.vtkPolyData()
Tubes4.DeepCopy(tubes.GetOutput())

# Finish the 4 pipelines
i = 1
while i < 5:
    idx = str(i)
    exec("mapEdges" + idx + " = vtk.vtkPolyDataMapper()")
    eval("mapEdges" + idx).SetInputData(eval("Tubes" + idx))

    exec("edgeActor" + idx + " = vtk.vtkActor()")
    eval("edgeActor" + idx).SetMapper(eval("mapEdges" + idx))
    eval("edgeActor" + idx).GetProperty().SetColor(GetRGBColor('peacock'))
    eval("edgeActor" + idx).GetProperty().SetSpecularColor(1, 1, 1)
    eval("edgeActor" + idx).GetProperty().SetSpecular(0.3)
    eval("edgeActor" + idx).GetProperty().SetSpecularPower(20)
    eval("edgeActor" + idx).GetProperty().SetAmbient(0.2)
    eval("edgeActor" + idx).GetProperty().SetDiffuse(0.8)

    exec("ren" + idx + " = vtk.vtkRenderer()")
    eval("ren" + idx).AddActor(eval("edgeActor" + idx))
    eval("ren" + idx).SetBackground(0, 0, 0)
    eval("ren" + idx).ResetCamera()
    eval("ren" + idx).GetActiveCamera().Zoom(1)
    eval("ren" + idx).GetActiveCamera().SetPosition(
      1.73906, 12.7987, -0.257808)
    eval("ren" + idx).GetActiveCamera().SetViewUp(
      0.992444, 0.00890284, -0.122379)
    eval("ren" + idx).GetActiveCamera().SetClippingRange(9.36398, 15.0496)
    i += 1

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

renWin.SetSize(600, 300)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren1.SetViewport(.75, 0, 1, 1)
ren2.SetViewport(.50, 0, .75, 1)
ren3.SetViewport(.25, 0, .50, 1)
ren4.SetViewport(0, 0, .25, 1)

# render the image
#
iren.Initialize()
#iren.Start()
