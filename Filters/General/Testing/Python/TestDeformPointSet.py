#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkPolyData,
)
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersGeneral import vtkDeformPointSet
from vtkmodules.vtkFiltersSources import vtkSphereSource
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

# Create the RenderWindow, Renderer and both Actors
#
renderer = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(renderer)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a sphere to warp
sphere = vtkSphereSource()
sphere.SetThetaResolution(51)
sphere.SetPhiResolution(17)

# Generate some scalars on the sphere
ele = vtkElevationFilter()
ele.SetInputConnection(sphere.GetOutputPort())
ele.SetLowPoint(0, 0, -0.5)
ele.SetHighPoint(0, 0, 0.5)

# Now create a control mesh, in this case a octagon
pts = vtkPoints()
pts.SetNumberOfPoints(6)
pts.SetPoint(0, -1, 0, 0)
pts.SetPoint(1, 1, 0, 0)
pts.SetPoint(2, 0, -1, 0)
pts.SetPoint(3, 0, 1, 0)
pts.SetPoint(4, 0, 0, -1)
pts.SetPoint(5, 0, 0, 1)

tris = vtkCellArray()
tris.InsertNextCell(3)
tris.InsertCellPoint(2)
tris.InsertCellPoint(0)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(2)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(3)
tris.InsertCellPoint(4)
tris.InsertNextCell(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(2)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(2)
tris.InsertCellPoint(1)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(1)
tris.InsertCellPoint(3)
tris.InsertCellPoint(5)
tris.InsertNextCell(3)
tris.InsertCellPoint(3)
tris.InsertCellPoint(0)
tris.InsertCellPoint(5)

pd = vtkPolyData()
pd.SetPoints(pts)
pd.SetPolys(tris)

# Display the control mesh
meshMapper = vtkPolyDataMapper()
meshMapper.SetInputData(pd)

meshActor = vtkActor()
meshActor.SetMapper(meshMapper)
meshActor.GetProperty().SetRepresentationToWireframe()
meshActor.GetProperty().SetColor(0, 0, 0)

# Okay now let's do the initial weight generation
deform = vtkDeformPointSet()
deform.SetInputConnection(ele.GetOutputPort())
deform.SetControlMeshData(pd)
deform.Update()  # this creates the initial weights

# Now move one point and deform
pts.SetPoint(5, 0, 0, 3)
pts.Modified()
deform.Update()

# Display the warped sphere
sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(deform.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)

renderer.AddActor(sphereActor)
renderer.AddActor(meshActor)
renderer.GetActiveCamera().SetPosition(1, 1, 1)
renderer.ResetCamera()

renderer.SetBackground(1, 1, 1)
renWin.SetSize(300, 300)

# interact with data
renWin.Render()
iren.Initialize()
# --- end of script --
