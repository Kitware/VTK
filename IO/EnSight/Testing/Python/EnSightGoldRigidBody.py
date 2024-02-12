#!/usr/bin/env python
from vtkmodules.vtkCommonExecutionModel import vtkCompositeDataPipeline
from vtkmodules.vtkCommonExecutionModel import vtkStreamingDemandDrivenPipeline
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCompositePolyDataMapper,
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

reader = vtkGenericEnSightReader()
# Make sure all algorithms use the composite data pipeline
cdp = vtkCompositeDataPipeline()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/veh.case")
reader.UpdateInformation()

outInfo = reader.GetOutputInformation(0)
numSteps = outInfo.Length(vtkStreamingDemandDrivenPipeline.TIME_STEPS())
if numSteps != 21:
    raise "Error: wrong number of time steps: %d. Should be 21" % numSteps

outInfo.Set(vtkStreamingDemandDrivenPipeline.UPDATE_TIME_STEP(), 36.0)
reader.Update()

geom0 = vtkGeometryFilter()
geom0.SetInputConnection(reader.GetOutputPort())
mapper0 = vtkCompositePolyDataMapper()
mapper0.SetInputConnection(geom0.GetOutputPort())
mapper0.SetColorModeToMapScalars()
mapper0.SetScalarModeToUseCellFieldData()
mapper0.ColorByArrayComponent("evect",0)
actor0 = vtkActor()
actor0.SetMapper(mapper0)
# assign our actor to the renderer
ren1.AddActor(actor0)

cam1 = ren1.GetActiveCamera()
cam1.SetFocalPoint(1.5, 0, -6.5)
cam1.SetPosition(26.4, 2.7, 1.4)
cam1.SetViewUp(-0.1, 1.0, -0.02)

# enable user interface interactor
iren.Initialize()
# prevent the tk window from showing up then start the event loop
reader.SetDefaultExecutivePrototype(None)
# --- end of script --
