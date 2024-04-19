#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersGeneral import vtkRemovePolyData
from vtkmodules.vtkFiltersSources import vtkPlaneSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the removal of polydata with exact matching.

# Control test size
res = 3

# Create a grid of cells
plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

# Mark the cells to be deleted - enforce exact match.
pd = vtkPolyData()
pd.SetPoints(plane.GetOutput().GetPoints())
cells = vtkCellArray()
cell = [0,1,5,4]
cells.InsertNextCell(4,cell)
cell = [10,11,14]
cells.InsertNextCell(3,cell)
pd.SetPolys(cells)

# Remove the cells
remove1 = vtkRemovePolyData()
remove1.AddInputConnection(plane.GetOutputPort())
remove1.AddInputData(pd)
remove1.ExactMatchOn()
remove1.Update()

m1 = vtkPolyDataMapper()
m1.SetInputConnection(remove1.GetOutputPort())

a1 = vtkActor()
a1.SetMapper(m1)

# Turn off exact matching
remove2 = vtkRemovePolyData()
remove2.AddInputConnection(plane.GetOutputPort())
remove2.AddInputData(pd)
remove2.ExactMatchOff()
remove2.Update()

m2 = vtkPolyDataMapper()
m2.SetInputConnection(remove2.GetOutputPort())

a2 = vtkActor()
a2.SetMapper(m2)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.5,1.0)
ren2 = vtkRenderer()
ren2.SetViewport(0.5,0,1.0,1.0)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(a1)
ren2.AddActor(a2)

renWin.SetSize(600,150)
ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)
ren2.SetActiveCamera(ren1.GetActiveCamera())

ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()
iren.Start()
