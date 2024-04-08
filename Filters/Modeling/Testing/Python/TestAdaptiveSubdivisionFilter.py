#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkGenerateIds
from vtkmodules.vtkFiltersModeling import vtkAdaptiveSubdivisionFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create a pipeline: some skinny-ass triangles
sphere = vtkSphereSource()
sphere.SetThetaResolution(6)
sphere.SetPhiResolution(24)

ids = vtkGenerateIds()
ids.SetInputConnection(sphere.GetOutputPort())

adapt = vtkAdaptiveSubdivisionFilter()
adapt.SetInputConnection(ids.GetOutputPort())
adapt.SetMaximumEdgeLength(0.1)
#adapt.SetMaximumTriangleArea(0.01)
#adapt.SetMaximumNumberOfPasses(1)
adapt.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(adapt.GetOutputPort())
mapper.SetScalarModeToUseCellFieldData()
mapper.SelectColorArray("vtkCellIds")
mapper.SetScalarRange(adapt.GetOutput().GetCellData().GetScalars().GetRange())

edgeProp = vtkProperty()
edgeProp.EdgeVisibilityOn()
edgeProp.SetEdgeColor(0,0,0)

actor = vtkActor()
actor.SetMapper(mapper)
actor.SetProperty(edgeProp)

# Graphics stuff
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(actor)
ren1.SetBackground(0,0,0)
ren1.GetActiveCamera().SetFocalPoint(0,0,0)
ren1.GetActiveCamera().SetPosition(0,0,1)
ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)

renWin.SetSize(300,300)
renWin.Render()
#iren.Start()
