#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkCompositeDataPipeline
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOEnSight import vtkEnSightMasterServerReader
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

# create a rendering window and renderer
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
renWin.StereoCapableWindowOn()
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
reader1 = vtkEnSightMasterServerReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtkCompositeDataPipeline()
reader1.SetDefaultExecutivePrototype(cdp)
reader1.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/mandelbrot.sos")
reader1.SetCurrentPiece(0)
geom0 = vtkGeometryFilter()
geom0.SetInputConnection(reader1.GetOutputPort())
mapper0 = vtkHierarchicalPolyDataMapper()
mapper0.SetInputConnection(geom0.GetOutputPort())
mapper0.SetColorModeToMapScalars()
mapper0.SetScalarModeToUsePointFieldData()
mapper0.ColorByArrayComponent("Iterations",0)
mapper0.SetScalarRange(0,112)
actor0 = vtkActor()
actor0.SetMapper(mapper0)
reader2 = vtkEnSightMasterServerReader()
reader2.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/mandelbrot.sos")
reader2.SetCurrentPiece(1)
geom2 = vtkGeometryFilter()
geom2.SetInputConnection(reader2.GetOutputPort())
mapper2 = vtkHierarchicalPolyDataMapper()
mapper2.SetInputConnection(geom2.GetOutputPort())
mapper2.SetColorModeToMapScalars()
mapper2.SetScalarModeToUsePointFieldData()
mapper2.ColorByArrayComponent("Iterations",0)
mapper2.SetScalarRange(0,112)
actor2 = vtkActor()
actor2.SetMapper(mapper2)
# assign our actor to the renderer
ren1.AddActor(actor0)
ren1.AddActor(actor2)
# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader1.SetDefaultExecutivePrototype(None)
# --- end of script --
