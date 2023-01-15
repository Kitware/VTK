#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonExecutionModel import (
    vtkAlgorithm,
    vtkCompositeDataPipeline,
)
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOEnSight import vtkEnSightGoldBinaryReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkHierarchicalPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# we need to use composite data pipeline with multiblock datasets
alg = vtkAlgorithm()
pip = vtkCompositeDataPipeline()
alg.SetDefaultExecutivePrototype(pip)
del pip
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtkEnSightGoldBinaryReader()
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/naca.bin.case")
reader.SetTimeValue(3)
lut = vtkLookupTable()
lut.SetHueRange(0.667,0.0)
lut.SetTableRange(0.636,1.34)
geom = vtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
blockMapper0 = vtkHierarchicalPolyDataMapper()
blockMapper0.SetInputConnection(geom.GetOutputPort())
blockActor0 = vtkActor()
blockActor0.SetMapper(blockMapper0)
ren1.AddActor(blockActor0)
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.SetFocalPoint(0,0,0)
cam1.ParallelProjectionOff()
cam1.Zoom(70)
cam1.SetViewAngle(1.0)
renWin.SetSize(400,400)
iren.Initialize()
alg.SetDefaultExecutivePrototype(None)
# --- end of script --
