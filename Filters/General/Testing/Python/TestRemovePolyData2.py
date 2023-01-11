#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkIdTypeArray
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

# Test the removal of polydata using cell ids, point ids, and
# cells, and a combination of all three.

# Control test size
res = 3

# Create a grid of cells
plane = vtkPlaneSource()
plane.SetResolution(res,res)
plane.Update()

# Mark the cells to be deleted. Add one bogus cell
# that should not be deleted.
pd = vtkPolyData()
pd.SetPoints(plane.GetOutput().GetPoints())
cells = vtkCellArray()
npts = 4
cell = [1,2,6,5]
cells.InsertNextCell(npts,cell)
cell = [1,2,6,4]
cells.InsertNextCell(npts,cell)
cell = [4,5,9,8]
cells.InsertNextCell(npts,cell)
cell = [6,7,11,10]
cells.InsertNextCell(npts,cell)
cell = [9,10,14,13]
cells.InsertNextCell(npts,cell)
cell = [0,3,15,12]
cells.InsertNextCell(npts,cell)
pd.SetPolys(cells)

# Subtract the cells
remove1 = vtkRemovePolyData()
remove1.AddInputConnection(plane.GetOutputPort())
remove1.AddInputData(pd)
remove1.Update()

m1 = vtkPolyDataMapper()
m1.SetInputConnection(remove1.GetOutputPort())

a1 = vtkActor()
a1.SetMapper(m1)

# Now remove using point ids
ptIds = vtkIdTypeArray()
ptIds.SetNumberOfTuples(4)
ptIds.SetTuple1(0,0)
ptIds.SetTuple1(1,3)
ptIds.SetTuple1(2,12)
ptIds.SetTuple1(3,15)

remove2 = vtkRemovePolyData()
remove2.AddInputConnection(plane.GetOutputPort())
remove2.SetPointIds(ptIds)
remove2.Update()

m2 = vtkPolyDataMapper()
m2.SetInputConnection(remove2.GetOutputPort())

a2 = vtkActor()
a2.SetMapper(m2)

# Now remove using cell ids
cellIds = vtkIdTypeArray()
cellIds.SetNumberOfTuples(1)
cellIds.SetTuple1(0,4)

remove3 = vtkRemovePolyData()
remove3.AddInputConnection(plane.GetOutputPort())
remove3.SetCellIds(cellIds)
remove3.Update()

m3 = vtkPolyDataMapper()
m3.SetInputConnection(remove3.GetOutputPort())

a3 = vtkActor()
a3.SetMapper(m3)

# Now remove using a combination - should produce nothing
remove4 = vtkRemovePolyData()
remove4.AddInputConnection(plane.GetOutputPort())
remove4.AddInputData(pd)
remove4.SetCellIds(cellIds)
remove4.SetPointIds(ptIds)
remove4.Update()

m4 = vtkPolyDataMapper()
m4.SetInputConnection(remove4.GetOutputPort())

a4 = vtkActor()
a4.SetMapper(m4)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.25,1.0)
ren2 = vtkRenderer()
ren2.SetViewport(0.25,0,0.5,1.0)
ren3 = vtkRenderer()
ren3.SetViewport(0.5,0,0.75,1.0)
ren4 = vtkRenderer()
ren4.SetViewport(0.75,0,1.0,1.0)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(a1)
ren2.AddActor(a2)
ren3.AddActor(a3)
ren4.AddActor(a4)

renWin.SetSize(600,150)
ren1.SetBackground(0.1, 0.2, 0.4)
ren2.SetBackground(0.1, 0.2, 0.4)
ren3.SetBackground(0.1, 0.2, 0.4)
ren4.SetBackground(0.1, 0.2, 0.4)
ren2.SetActiveCamera(ren1.GetActiveCamera())
ren3.SetActiveCamera(ren1.GetActiveCamera())
ren4.SetActiveCamera(ren1.GetActiveCamera())

ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()

iren.Initialize()

# render the image
#
renWin.Render()
iren.Start()
