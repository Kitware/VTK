#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkAppendPolyData
from vtkmodules.vtkFiltersHybrid import vtkDepthSortPolyData
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    VTK_SCALAR_MODE_USE_CELL_FIELD_DATA,
    vtkActor,
    vtkCamera,
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
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
camera = vtkCamera()
ren1.SetActiveCamera(camera)
# create a sphere source and actor
#
sphere = vtkSphereSource()
sphere.SetThetaResolution(80)
sphere.SetPhiResolution(40)
sphere.SetRadius(1)
sphere.SetCenter(0,0,0)
sphere2 = vtkSphereSource()
sphere2.SetThetaResolution(80)
sphere2.SetPhiResolution(40)
sphere2.SetRadius(0.5)
sphere2.SetCenter(1,0,0)
sphere3 = vtkSphereSource()
sphere3.SetThetaResolution(80)
sphere3.SetPhiResolution(40)
sphere3.SetRadius(0.5)
sphere3.SetCenter(-1,0,0)
sphere4 = vtkSphereSource()
sphere4.SetThetaResolution(80)
sphere4.SetPhiResolution(40)
sphere4.SetRadius(0.5)
sphere4.SetCenter(0,1,0)
sphere5 = vtkSphereSource()
sphere5.SetThetaResolution(80)
sphere5.SetPhiResolution(40)
sphere5.SetRadius(0.5)
sphere5.SetCenter(0,-1,0)
appendData = vtkAppendPolyData()
appendData.AddInputConnection(sphere.GetOutputPort())
appendData.AddInputConnection(sphere2.GetOutputPort())
appendData.AddInputConnection(sphere3.GetOutputPort())
appendData.AddInputConnection(sphere4.GetOutputPort())
appendData.AddInputConnection(sphere5.GetOutputPort())
depthSort = vtkDepthSortPolyData()
depthSort.SetInputConnection(appendData.GetOutputPort())
depthSort.SetDirectionToBackToFront()
depthSort.SetVector(1,1,1)
depthSort.SetCamera(camera)
depthSort.SortScalarsOn()
depthSort.Update()
mapper = vtkPolyDataMapper()
mapper.SetInputConnection(depthSort.GetOutputPort())
mapper.SetScalarRange(0,depthSort.GetOutput().GetNumberOfCells())
mapper.SetScalarVisibility(1);
mapper.SelectColorArray("sortedCellIds");
mapper.SetUseLookupTableScalarRange(0);
mapper.SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetOpacity(0.5)
actor.GetProperty().SetColor(1,0,0)
actor.RotateX(-72)
depthSort.SetProp3D(actor)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(300,200)
# render the image
#
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(2.2)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
