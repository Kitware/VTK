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
from vtkmodules.vtkFiltersGeneral import vtkImageDataToPointSet
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOXML import vtkXMLRectilinearGridReader
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
from vtkmodules.test import Testing
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()

# Control debugging parameters
res = 50

# Create the RenderWindow, Renderers
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

# Converts image data to structured grid
convert = vtkImageDataToPointSet()
convert.SetInputConnection(sample.GetOutputPort())
convert.Update()

cthvtr = vtkXMLRectilinearGridReader()
cthvtr.SetFileName(VTK_DATA_ROOT + "/Data/cth.vtr")
cthvtr.CellArrayStatus = ['Pressure', 'Void Volume Fraction', 'X Velocity', 'Y Velocity', 'Z Velocity',
                          'Volume Fraction for Armor Plate', 'Mass for Armor Plate', 'Volume Fraction for Body, Nose',
                          'Mass for Body, Nose']
cthvtr.Update()
input = cthvtr.GetOutput()

# Create a cutting plane
plane = vtkPlane()
plane.SetOrigin(input.GetCenter())
plane.SetNormal(1, 1, 1)

# First create the usual cutter
cutter = vtkCutter()
cutter.SetInputData(input)
cutter.SetCutFunction(plane)

cutterMapper = vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())
cutterMapper.ScalarVisibilityOff()

cutterActor = vtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1, 1, 1)

# Throw in an outline
outline = vtkOutlineFilter()
outline.SetInputData(input)

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the accelerated version.
sCutter = vtkPlaneCutter()
sCutter.SetInputData(input)
sCutter.SetPlane(plane)

sCutterMapper = vtkPolyDataMapper()
sCutterMapper.SetInputConnection(sCutter.GetOutputPort())
sCutterMapper.ScalarVisibilityOff()

sCutterActor = vtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1, 1, 1)

# Accelerated cutter without tree
snCutter = vtkPlaneCutter()
snCutter.SetInputData(input)
snCutter.SetPlane(plane)
snCutter.BuildTreeOff()

snCutterMapper = vtkPolyDataMapper()
snCutterMapper.SetInputConnection(snCutter.GetOutputPort())
snCutterMapper.ScalarVisibilityOff()

snCutterActor = vtkActor()
snCutterActor.SetMapper(snCutterMapper)
snCutterActor.GetProperty().SetColor(1, 1, 1)

outlineT = vtkOutlineFilter()
outlineT.SetInputData(input)

outlineMapperT = vtkPolyDataMapper()
outlineMapperT.SetInputConnection(outlineT.GetOutputPort())

outlineActorT = vtkActor()
outlineActorT.SetMapper(outlineMapperT)

# Time the execution of the filter w/out scalar tree
cutter_timer = vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print("vtkCutter:", CT)

# Time the execution of the filter w/o sphere tree
snCutter_timer = vtkExecutionTimer()
snCutter_timer.SetFilter(snCutter)
snCutter.Update()
ST = snCutter_timer.GetElapsedWallClockTime()
print("vtkPlaneCutter: Without sphere tree:", ST)

# Time the execution of the filter w/ sphere tree
sCutter_timer = vtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
sCutter.Update()
ST = sCutter_timer.GetElapsedWallClockTime()
print("vtkPlaneCutter: Build sphere tree + execute once:", ST)

sCutter_timer = vtkExecutionTimer()
sCutter_timer.SetFilter(sCutter)
plane.Modified()
sCutter.Update()
SC = sCutter_timer.GetElapsedWallClockTime()
print("vtkPlaneCutter: with prebuilt sphere-tree", SC)

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
