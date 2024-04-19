#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkFiltersCore import vtkPolyDataNormals
from vtkmodules.vtkFiltersGeneral import (
    vtkMultiBlockDataGroupFilter,
    vtkRandomAttributeGenerator,
)
from vtkmodules.vtkFiltersSources import (
    vtkPlaneSource,
    vtkSphereSource,
)
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

def PlaneSphereActors():
    ps = vtkPlaneSource()
    ps.SetXResolution(10)
    ps.SetYResolution(10)

    ss = vtkSphereSource()
    ss.SetRadius (0.3)

    group = vtkMultiBlockDataGroupFilter()
    group.AddInputConnection(ps.GetOutputPort())
    group.AddInputConnection(ss.GetOutputPort())

    ag = vtkRandomAttributeGenerator()
    ag.SetInputConnection(group.GetOutputPort())
    ag.GenerateCellScalarsOn()
    ag.AttributesConstantPerBlockOn()

    n = vtkPolyDataNormals()
    n.SetInputConnection(ag.GetOutputPort())
    n.Update ();

    actors = []
    it = n.GetOutputDataObject(0).NewIterator()
    it.InitTraversal()
    while not it.IsDoneWithTraversal():
        pm = vtkPolyDataMapper()
        pm.SetInputData(it.GetCurrentDataObject())

        a = vtkActor()
        a.SetMapper(pm)
        actors.append (a)
        it.GoToNextItem()
    return actors

# Create the RenderWindow, Renderer and interactive renderer
ren = vtkRenderer()
ren.SetBackground(0, 0, 0)
renWin = vtkRenderWindow()
renWin.SetSize(300, 300)
renWin.AddRenderer(ren)

# make sure to have the same regression image on all platforms.
renWin.SetMultiSamples(0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Force a starting random value
raMath = vtkMath()
raMath.RandomSeed(6)

# Generate random cell attributes on a plane and a cylinder
for a in PlaneSphereActors():
    ren.AddActor(a)

# reorient the camera
camera = ren.GetActiveCamera()
camera.Azimuth(20)
camera.Elevation(20)
ren.SetActiveCamera(camera)
ren.ResetCamera()

renWin.Render()
#iren.Start()
