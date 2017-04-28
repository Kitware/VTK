#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# we need to use composite data pipeline with multiblock datasets
alg = vtk.vtkAlgorithm()
pip = vtk.vtkCompositeDataPipeline()
alg.SetDefaultExecutivePrototype(pip)
del pip
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtk.vtkEnSightGoldBinaryReader()
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/naca.bin.case")
reader.SetTimeValue(3)
lut = vtk.vtkLookupTable()
lut.SetHueRange(0.667,0.0)
lut.SetTableRange(0.636,1.34)
geom = vtk.vtkGeometryFilter()
geom.SetInputConnection(reader.GetOutputPort())
blockMapper0 = vtk.vtkHierarchicalPolyDataMapper()
blockMapper0.SetInputConnection(geom.GetOutputPort())
blockActor0 = vtk.vtkActor()
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
