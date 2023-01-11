#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtkCutter,
    vtkExecutionTimer,
    vtkPlaneCutter,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersGeneral import vtkRandomAttributeGenerator
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
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

res = 50

# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = vtkSphere()
sphere.SetCenter(0.0, 0.0, 0.0)
sphere.SetRadius(0.25)

sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5, 0.5, -0.5, 0.5, -0.5, 0.5)
sample.SetSampleDimensions(res, res, res)
sample.Update()

# Adds random attributes
random = vtkRandomAttributeGenerator()
random.SetGenerateCellScalars(True)
random.SetInputConnection(sample.GetOutputPort())

# Convert the image data to unstructured grid
extractionSphere = vtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0, 0, 0)
extract = vtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(random.GetOutputPort())
extract.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0, 0, 0)
plane.SetNormal(1, 1, 1)

# Now create the usual cutter
cutter = vtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)

cutterMapper = vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

cutterActor = vtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1, 1, 1)

# Throw in an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Accelerated cutter
cut = vtkPlaneCutter()
cut.SetInputConnection(extract.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()

sCutterMapper = vtkPolyDataMapper()
sCutterMapper.SetInputConnection(cut.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = vtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1, 1, 1)

# Accelerated cutter without tree
ncut = vtkPlaneCutter()
ncut.SetInputConnection(extract.GetOutputPort())
ncut.SetPlane(plane)
ncut.ComputeNormalsOff()
ncut.BuildTreeOff()

snCutterMapper = vtkPolyDataMapper()
snCutterMapper.SetInputConnection(ncut.GetOutputPort())
snCutterMapper.ScalarVisibilityOff()

snCutterActor = vtkActor()
snCutterActor.SetMapper(snCutterMapper)
snCutterActor.GetProperty().SetColor(1, 1, 1)

outlineT = vtkOutlineFilter()
outlineT.SetInputConnection(sample.GetOutputPort())

outlineMapperT = vtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = vtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the usual cutter
cutter_timer = vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print("vtkCutter:", CT)

# Time the execution of the filter w/ sphere tree
sCutter_timer = vtkExecutionTimer()
sCutter_timer.SetFilter(cut)
cut.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print("Build sphere tree + execute once:", ST)

# Time subsequent cuts
sCutter_timer.SetFilter(cut)
plane.Modified()
cut.Update()
SC = sCutter_timer.GetElapsedWallClockTime()
print("vtkPlaneCutter:", SC)

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(outlineActorT)
ren1.AddActor(sCutterActor)
ren2.AddActor(outlineActorT)
ren2.AddActor(snCutterActor)

ren0.SetBackground(0, 0, 0)
ren1.SetBackground(0, 0, 0)
ren2.SetBackground(0, 0, 0)
ren0.SetViewport(0, 0, 0.33, 1)
ren1.SetViewport(0.33, 0, 0.66, 1)
ren2.SetViewport(0.66, 0, 1, 1)
renWin.SetSize(900, 300)
ren0.ResetCamera()
ren1.ResetCamera()
ren2.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
