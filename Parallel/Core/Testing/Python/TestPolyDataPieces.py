#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkCommonDataModel import vtkDataSetAttributes
from vtkmodules.vtkFiltersCore import vtkPolyDataNormals
from vtkmodules.vtkFiltersParallel import (
    vtkExtractPolyDataPiece,
    vtkPieceScalars,
    vtkPipelineSize,
)
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

math = vtkMath()
math.RandomSeed(22)

sphere = vtkSphereSource()
sphere.SetPhiResolution(32)
sphere.SetThetaResolution(32)

extract = vtkExtractPolyDataPiece()
extract.SetInputConnection(sphere.GetOutputPort())

normals = vtkPolyDataNormals()
normals.SetInputConnection(extract.GetOutputPort())

ps = vtkPieceScalars()
ps.SetInputConnection(normals.GetOutputPort())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(2)

actor = vtkActor()
actor.SetMapper(mapper)

sphere2 = vtkSphereSource()
sphere2.SetPhiResolution(32)
sphere2.SetThetaResolution(32)

extract2 = vtkExtractPolyDataPiece()
extract2.SetInputConnection(sphere2.GetOutputPort())

mapper2 = vtkPolyDataMapper()
mapper2.SetInputConnection(extract2.GetOutputPort())
mapper2.SetNumberOfPieces(2)
mapper2.SetPiece(1)
mapper2.SetScalarRange(0, 4)
mapper2.SetScalarModeToUseCellFieldData()
mapper2.SetColorModeToMapScalars()
mapper2.ColorByArrayComponent(vtkDataSetAttributes.GhostArrayName(), 0)
mapper2.SetGhostLevel(4)

# check the pipeline size
extract2.UpdateInformation()
psize = vtkPipelineSize()
if (psize.GetEstimatedSize(extract2, 0, 0) > 100):
    print ("ERROR: Pipeline Size increased")
if (psize.GetNumberOfSubPieces(10, mapper2, mapper2.GetPiece(), mapper2.GetNumberOfPieces()) != 1):
    print ("ERROR: Number of sub pieces changed",
           psize.GetNumberOfSubPieces(10, mapper2, mapper2.GetPiece(), mapper2.GetNumberOfPieces()))

actor2 = vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5, 0, 0)

sphere3 = vtkSphereSource()
sphere3.SetPhiResolution(32)
sphere3.SetThetaResolution(32)

extract3 = vtkExtractPolyDataPiece()
extract3.SetInputConnection(sphere3.GetOutputPort())

ps3 = vtkPieceScalars()
ps3.SetInputConnection(extract3.GetOutputPort())

mapper3 = vtkPolyDataMapper()
mapper3.SetInputConnection(ps3.GetOutputPort())
mapper3.SetNumberOfSubPieces(8)
mapper3.SetScalarRange(0, 8)

actor3 = vtkActor()
actor3.SetMapper(mapper3)
actor3.SetPosition(0, -1.5, 0)

sphere4 = vtkSphereSource()
sphere4.SetPhiResolution(32)
sphere4.SetThetaResolution(32)

extract4 = vtkExtractPolyDataPiece()
extract4.SetInputConnection(sphere4.GetOutputPort())

ps4 = vtkPieceScalars()
ps4.RandomModeOn()
ps4.SetScalarModeToCellData()
ps4.SetInputConnection(extract4.GetOutputPort())

mapper4 = vtkPolyDataMapper()
mapper4.SetInputConnection(ps4.GetOutputPort())
mapper4.SetNumberOfSubPieces(8)
mapper4.SetScalarRange(0, 8)

actor4 = vtkActor()
actor4.SetMapper(mapper4)
actor4.SetPosition(1.5, -1.5, 0)

ren = vtkRenderer()
ren.AddActor(actor)
ren.AddActor(actor2)
ren.AddActor(actor3)
ren.AddActor(actor4)

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
#iren.Start()
