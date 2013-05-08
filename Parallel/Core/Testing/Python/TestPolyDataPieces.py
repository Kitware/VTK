#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

math = vtk.vtkMath()
math.RandomSeed(22)

sphere = vtk.vtkSphereSource()
sphere.SetPhiResolution(32)
sphere.SetThetaResolution(32)

extract = vtk.vtkExtractPolyDataPiece()
extract.SetInputConnection(sphere.GetOutputPort())

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(extract.GetOutputPort())

ps = vtk.vtkPieceScalars()
ps.SetInputConnection(normals.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(2)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

sphere2 = vtk.vtkSphereSource()
sphere2.SetPhiResolution(32)
sphere2.SetThetaResolution(32)

extract2 = vtk.vtkExtractPolyDataPiece()
extract2.SetInputConnection(sphere2.GetOutputPort())

mapper2 = vtk.vtkPolyDataMapper()
mapper2.SetInputConnection(extract2.GetOutputPort())
mapper2.SetNumberOfPieces(2)
mapper2.SetPiece(1)
mapper2.SetScalarRange(0, 4)
mapper2.SetScalarModeToUseCellFieldData()
mapper2.SetColorModeToMapScalars()
mapper2.ColorByArrayComponent("vtkGhostLevels", 0)
mapper2.SetGhostLevel(4)

# check the pipeline size
extract2.UpdateInformation()
psize = vtk.vtkPipelineSize()
if (psize.GetEstimatedSize(extract2, 0, 0) > 100):
    print ("ERROR: Pipeline Size increased")
    pass
if (psize.GetNumberOfSubPieces(10, mapper2) != 2):
    print ("ERROR: Number of sub pieces changed")
    pass

actor2 = vtk.vtkActor()
actor2.SetMapper(mapper2)
actor2.SetPosition(1.5, 0, 0)

sphere3 = vtk.vtkSphereSource()
sphere3.SetPhiResolution(32)
sphere3.SetThetaResolution(32)

extract3 = vtk.vtkExtractPolyDataPiece()
extract3.SetInputConnection(sphere3.GetOutputPort())

ps3 = vtk.vtkPieceScalars()
ps3.SetInputConnection(extract3.GetOutputPort())

mapper3 = vtk.vtkPolyDataMapper()
mapper3.SetInputConnection(ps3.GetOutputPort())
mapper3.SetNumberOfSubPieces(8)
mapper3.SetScalarRange(0, 8)

actor3 = vtk.vtkActor()
actor3.SetMapper(mapper3)
actor3.SetPosition(0, -1.5, 0)

sphere4 = vtk.vtkSphereSource()
sphere4.SetPhiResolution(32)
sphere4.SetThetaResolution(32)

extract4 = vtk.vtkExtractPolyDataPiece()
extract4.SetInputConnection(sphere4.GetOutputPort())

ps4 = vtk.vtkPieceScalars()
ps4.RandomModeOn()
ps4.SetScalarModeToCellData()
ps4.SetInputConnection(extract4.GetOutputPort())

mapper4 = vtk.vtkPolyDataMapper()
mapper4.SetInputConnection(ps4.GetOutputPort())
mapper4.SetNumberOfSubPieces(8)
mapper4.SetScalarRange(0, 8)

actor4 = vtk.vtkActor()
actor4.SetMapper(mapper4)
actor4.SetPosition(1.5, -1.5, 0)

ren = vtk.vtkRenderer()
ren.AddActor(actor)
ren.AddActor(actor2)
ren.AddActor(actor3)
ren.AddActor(actor4)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()
#iren.Start()
