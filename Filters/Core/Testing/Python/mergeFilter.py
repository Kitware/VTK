#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import (
    vtkMergeFilter,
    vtkProbeFilter,
    vtkStructuredGridOutlineFilter,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkWarpScalar
from vtkmodules.vtkFiltersSources import vtkLineSource
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(110)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)
probeLine = vtkLineSource()
probeLine.SetPoint1(1,1,29)
probeLine.SetPoint2(16.5,5,31.7693)
probeLine.SetResolution(500)
probe = vtkProbeFilter()
probe.SetInputConnection(probeLine.GetOutputPort())
probe.SetSourceData(output)
probe.Update()
probeTube = vtkTubeFilter()
probeTube.SetInputData(probe.GetPolyDataOutput())
probeTube.SetNumberOfSides(5)
probeTube.SetRadius(.05)
probeMapper = vtkPolyDataMapper()
probeMapper.SetInputConnection(probeTube.GetOutputPort())
probeMapper.SetScalarRange(output.GetScalarRange())
probeActor = vtkActor()
probeActor.SetMapper(probeMapper)
displayLine = vtkLineSource()
displayLine.SetPoint1(0,0,0)
displayLine.SetPoint2(1,0,0)
displayLine.SetResolution(probeLine.GetResolution())
displayMerge = vtkMergeFilter()
displayMerge.SetGeometryConnection(displayLine.GetOutputPort())
displayMerge.SetScalarsData(probe.GetPolyDataOutput())
displayMerge.Update()
displayWarp = vtkWarpScalar()
displayWarp.SetInputData(displayMerge.GetPolyDataOutput())
displayWarp.SetNormal(0,1,0)
displayWarp.SetScaleFactor(.000001)
displayWarp.Update()
displayMapper = vtkPolyDataMapper()
displayMapper.SetInputData(displayWarp.GetPolyDataOutput())
displayMapper.SetScalarRange(output.GetScalarRange())
displayActor = vtkActor()
displayActor.SetMapper(displayMapper)
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
ren1.AddActor(outlineActor)
ren1.AddActor(probeActor)
ren1.SetBackground(1,1,1)
ren1.SetViewport(0,.25,1,1)
ren2.AddActor(displayActor)
ren2.SetBackground(0,0,0)
ren2.SetViewport(0,0,1,.25)
renWin.SetSize(300,300)
ren1.ResetCamera()
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(9.9,-26,41)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
ren2.ResetCamera()
cam2 = ren2.GetActiveCamera()
cam2.ParallelProjectionOn()
cam2.SetParallelScale(.15)
iren.Initialize()
# render the image
#
# prevent the tk window from showing up then start the event loop
# --- end of script --
