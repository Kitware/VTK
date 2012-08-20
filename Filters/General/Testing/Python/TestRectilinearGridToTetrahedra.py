#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

### SetUp the pipeline
FormMesh = vtk.vtkRectilinearGridToTetrahedra()
FormMesh.SetInput(4,2,2,1,1,1,0.001)
FormMesh.RememberVoxelIdOn()
TetraEdges = vtk.vtkExtractEdges()
TetraEdges.SetInputConnection(FormMesh.GetOutputPort())
tubes = vtk.vtkTubeFilter()
tubes.SetInputConnection(TetraEdges.GetOutputPort())
tubes.SetRadius(0.05)
tubes.SetNumberOfSides(6)
### Run the pipeline 3 times, with different conversions to TetMesh
Tubes1 = vtk.vtkPolyData()
FormMesh.SetTetraPerCellTo5()
tubes.Update()
Tubes1.DeepCopy(tubes.GetOutput())
Tubes2 = vtk.vtkPolyData()
FormMesh.SetTetraPerCellTo6()
tubes.Update()
Tubes2.DeepCopy(tubes.GetOutput())
Tubes3 = vtk.vtkPolyData()
FormMesh.SetTetraPerCellTo12()
tubes.Update()
Tubes3.DeepCopy(tubes.GetOutput())
### Run the pipeline once more, this time converting some cells to
### 5 and some data to 12 TetMesh
### Determine which cells are which
DivTypes = vtk.vtkIntArray()
numCell = FormMesh.GetInput().GetNumberOfCells()
DivTypes.SetNumberOfValues(numCell)
i = 0
while i < numCell:
    DivTypes.SetValue(i,expr.expr(globals(), locals(),["5","+","(","7","*","(","i","%","4","))"]))
    i = i + 1

### Finish this pipeline
Tubes4 = vtk.vtkPolyData()
FormMesh.SetTetraPerCellTo5And12()
FormMesh.GetInput().GetCellData().SetScalars(DivTypes)
tubes.Update()
Tubes4.DeepCopy(tubes.GetOutput())
### Finish the 4 pipelines
i = 1
while i < 5:
    locals()[get_variable_name("mapEdges", i, "")] = vtk.vtkPolyDataMapper()
    locals()[get_variable_name("mapEdges", i, "")].SetInputData(locals()[get_variable_name("Tubes", i, "")])
    locals()[get_variable_name("edgeActor", i, "")] = vtk.vtkActor()
    locals()[get_variable_name("edgeActor", i, "")].SetMapper(locals()[get_variable_name("mapEdges", i, "")])
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetColor(peacock)
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetSpecularColor(1,1,1)
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetSpecular(0.3)
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetSpecularPower(20)
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetAmbient(0.2)
    locals()[get_variable_name("edgeActor", i, "")].GetProperty().SetDiffuse(0.8)
    locals()[get_variable_name("ren", i, "")] = vtk.vtkRenderer()
    locals()[get_variable_name("ren", i, "")].AddActor(locals()[get_variable_name("edgeActor", i, "")])
    locals()[get_variable_name("ren", i, "")].SetBackground(0,0,0)
    locals()[get_variable_name("ren", i, "")].ResetCamera()
    locals()[get_variable_name("ren", i, "")].GetActiveCamera().Zoom(1)
    locals()[get_variable_name("ren", i, "")].GetActiveCamera().SetPosition(1.73906,12.7987,-0.257808)
    locals()[get_variable_name("ren", i, "")].GetActiveCamera().SetViewUp(0.992444,0.00890284,-0.122379)
    locals()[get_variable_name("ren", i, "")].GetActiveCamera().SetClippingRange(9.36398,15.0496)
    i = i + 1

# Create graphics objects
# Create the rendering window, renderer, and interactive renderer
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
renWin.SetSize(600,300)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# Add the actors to the renderer, set the background and size
ren1.SetViewport(.75,0,1,1)
ren2.SetViewport(.50,0,.75,1)
ren3.SetViewport(.25,0,.50,1)
ren4.SetViewport(0,0,.25,1)
# render the image
#
iren.Initialize()
# prevent the tk window from showing up then start the event loop
# --- end of script --
