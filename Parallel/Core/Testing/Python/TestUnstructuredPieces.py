#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkMath
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkPolyDataNormals,
)
from vtkmodules.vtkFiltersGeneral import vtkDataSetTriangleFilter
from vtkmodules.vtkFiltersParallel import (
    vtkExtractUnstructuredGridPiece,
    vtkPieceScalars,
)
from vtkmodules.vtkIOParallel import vtkMultiBlockPLOT3DReader
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

pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.Update()

output = pl3d.GetOutput().GetBlock(0)

dst = vtkDataSetTriangleFilter()
dst.SetInputData(output)

extract = vtkExtractUnstructuredGridPiece()
extract.SetInputConnection(dst.GetOutputPort())

cf = vtkContourFilter()
cf.SetInputConnection(extract.GetOutputPort())
cf.SetValue(0, 0.24)

pdn = vtkPolyDataNormals()
pdn.SetInputConnection(cf.GetOutputPort())

ps = vtkPieceScalars()
ps.SetInputConnection(pdn.GetOutputPort())

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(ps.GetOutputPort())
mapper.SetNumberOfPieces(3)

actor = vtkActor()
actor.SetMapper(mapper)

ren = vtkRenderer()
ren.AddActor(actor)
ren.ResetCamera()

camera = ren.GetActiveCamera()
# $camera SetPosition 68.1939 -23.4323 12.6465
# $camera SetViewUp 0.46563 0.882375 0.0678508
# $camera SetFocalPoint 3.65707 11.4552 1.83509
# $camera SetClippingRange 59.2626 101.825

renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
iren.Initialize()

#iren.Start()
