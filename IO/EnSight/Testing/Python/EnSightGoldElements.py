#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# create a rendering window and renderer
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader = vtk.vtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtk.vtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName("" + str(VTK_DATA_ROOT) + "/Data/EnSight/elements.case")
geom0 = vtk.vtkGeometryFilter()
geom0.SetInputConnection(reader.GetOutputPort())
mapper0 = vtk.vtkHierarchicalPolyDataMapper()
mapper0.SetInputConnection(geom0.GetOutputPort())
mapper0.SetColorModeToMapScalars()
mapper0.SetScalarModeToUsePointFieldData()
mapper0.ColorByArrayComponent("pointScalars",0)
mapper0.SetScalarRange(0,112)
actor0 = vtk.vtkActor()
actor0.SetMapper(mapper0)
# assign our actor to the renderer
ren1.AddActor(actor0)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
