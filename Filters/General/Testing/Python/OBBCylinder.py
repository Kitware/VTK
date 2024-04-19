#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import vtkExtractEdges
from vtkmodules.vtkFiltersGeneral import (
    vtkOBBTree,
    vtkSpatialRepresentationFilter,
    vtkTransformPolyDataFilter,
)
from vtkmodules.vtkFiltersSources import vtkCylinderSource
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

cylinder = vtkCylinderSource()
cylinder.SetHeight(1)
cylinder.SetRadius(4)
cylinder.SetResolution(100)
cylinder.CappingOff()

foo = vtkTransform()
foo.RotateX(20)
foo.RotateY(10)
foo.RotateZ(27)
foo.Scale(1, .7, .3)

transPD = vtkTransformPolyDataFilter()
transPD.SetInputConnection(cylinder.GetOutputPort())
transPD.SetTransform(foo)

dataMapper = vtkPolyDataMapper()
dataMapper.SetInputConnection(transPD.GetOutputPort())

model = vtkActor()
model.SetMapper(dataMapper)
model.GetProperty().SetColor(1, 0, 0)

obb = vtkOBBTree()
obb.SetMaxLevel(10)
obb.SetNumberOfCellsPerNode(5)
obb.AutomaticOff()

boxes = vtkSpatialRepresentationFilter()
boxes.SetInputConnection(transPD.GetOutputPort())
boxes.SetSpatialRepresentation(obb)
boxes.SetGenerateLeaves(1)
boxes.Update()

output = boxes.GetOutput().GetBlock(boxes.GetMaximumLevel() + 1)
boxEdges = vtkExtractEdges()
boxEdges.SetInputData(output)

boxMapper = vtkPolyDataMapper()
boxMapper.SetInputConnection(boxEdges.GetOutputPort())
boxMapper.SetResolveCoincidentTopology(1)

boxActor = vtkActor()
boxActor.SetMapper(boxMapper)
boxActor.GetProperty().SetAmbient(1)
boxActor.GetProperty().SetDiffuse(0)
boxActor.GetProperty().SetRepresentationToWireframe()

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(model)
ren1.AddActor(boxActor)
ren1.SetBackground(0.1, 0.2, 0.4)

renWin.SetSize(300, 300)

ren1.ResetCamera()
ren1.GetActiveCamera().Zoom(1.5)

# render the image
#
iren.Initialize()

#iren.Start()
