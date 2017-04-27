#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()


math = vtk.vtkMath()
math.RandomSeed(22)

pl3d = vtk.vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

dst = vtk.vtkDataSetTriangleFilter()
dst.SetInputData(output)

extract = vtk.vtkExtractUnstructuredGridPiece()
extract.SetInputConnection(dst.GetOutputPort())

cf = vtk.vtkContourFilter()
cf.SetInputConnection(extract.GetOutputPort())
cf.SetValue(0, 0.24)

pdn = vtk.vtkPolyDataNormals()
pdn.SetInputConnection(cf.GetOutputPort())

ps = vtk.vtkPieceScalars()
ps.SetInputConnection(pdn.GetOutputPort())

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(3)

actor = vtk.vtkActor()
actor.SetMapper(mapper)

ren = vtk.vtkRenderer()
ren.AddActor(actor)
ren.ResetCamera()

camera = ren.GetActiveCamera()
# $camera SetPosition 68.1939 -23.4323 12.6465
# $camera SetViewUp 0.46563 0.882375 0.0678508
# $camera SetFocalPoint 3.65707 11.4552 1.83509
# $camera SetClippingRange 59.2626 101.825

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()

#iren.Start()
