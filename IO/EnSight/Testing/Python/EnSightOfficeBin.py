#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkDataObject
from vtkmodules.vtkCommonExecutionModel import vtkCompositeDataPipeline
from vtkmodules.vtkFiltersCore import (
    vtkGlyph3D,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
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

ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# read data
#
reader = vtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/office_bin.case")
reader.Update()
outline = vtkStructuredGridOutlineFilter()
#    outline SetInputConnection [reader GetOutputPort]
outline.SetInputData(reader.GetOutput().GetBlock(0))
mapOutline = vtkPolyDataMapper()
mapOutline.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(mapOutline)
outlineActor.GetProperty().SetColor(0,0,0)
# Create source for streamtubes
streamer = vtkStreamTracer()
#    streamer SetInputConnection [reader GetOutputPort]
streamer.SetInputData(reader.GetOutput().GetBlock(0))
streamer.SetStartPosition(0.1,2.1,0.5)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.1)
streamer.SetIntegrationDirectionToForward()
cone = vtkConeSource()
cone.SetResolution(8)
cones = vtkGlyph3D()
cones.SetInputConnection(streamer.GetOutputPort())
cones.SetSourceConnection(cone.GetOutputPort())
cones.SetScaleFactor(3)
cones.SetInputArrayToProcess(1, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "vectors")
cones.SetScaleModeToScaleByVector()
mapCones = vtkPolyDataMapper()
mapCones.SetInputConnection(cones.GetOutputPort())
#    eval mapCones SetScalarRange [[reader GetOutput] GetScalarRange]
mapCones.SetScalarRange(reader.GetOutput().GetBlock(0).GetScalarRange())
conesActor = vtkActor()
conesActor.SetMapper(mapCones)
ren1.AddActor(outlineActor)
ren1.AddActor(conesActor)
ren1.SetBackground(0.4,0.4,0.5)
renWin.SetSize(300,300)
iren.Initialize()
# interact with data
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
