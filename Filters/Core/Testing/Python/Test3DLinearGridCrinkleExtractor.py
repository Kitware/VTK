#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtk3DLinearGridCrinkleExtractor,
    vtkExecutionTimer,
)
from vtkmodules.vtkFiltersExtraction import vtkExtractGeometry
from vtkmodules.vtkFiltersGeneral import vtkRandomAttributeGenerator
from vtkmodules.vtkFiltersGeometry import vtkGeometryFilter
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

# Control test size
res = 50
#res = 300

# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
ren1 = vtkRenderer()
ren2 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create a synthetic source: sample a sphere across a volume
sphere = vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

sample = vtkSampleFunction()
sample.SetImplicitFunction(sphere)
sample.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.Update()

# Adds random attributes
random = vtkRandomAttributeGenerator()
random.SetGenerateCellScalars(True)
random.SetInputConnection(sample.GetOutputPort())

# Convert the image data to unstructured grid
extractionSphere = vtkSphere()
extractionSphere.SetRadius(100)
extractionSphere.SetCenter(0,0,0)
extract = vtkExtractGeometry()
extract.SetImplicitFunction(extractionSphere)
extract.SetInputConnection(random.GetOutputPort())
extract.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(1,1,1)

# Now create the usual extractor
cutter = vtkExtractGeometry()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetImplicitFunction(plane)
cutter.ExtractBoundaryCellsOn()
cutter.ExtractOnlyBoundaryCellsOn()

cutterSurface = vtkGeometryFilter()
cutterSurface.SetInputConnection(cutter.GetOutputPort())
cutterSurface.MergingOff()

cutterMapper = vtkPolyDataMapper()
cutterMapper.SetInputConnection(cutterSurface.GetOutputPort())

cutterActor = vtkActor()
cutterActor.SetMapper(cutterMapper)
cutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create the faster crinkle cutter - cell data
pcut = vtk3DLinearGridCrinkleExtractor()
pcut.SetInputConnection(extract.GetOutputPort())
pcut.SetImplicitFunction(plane)
pcut.CopyPointDataOff()
pcut.CopyCellDataOn()

pCutterSurface = vtkGeometryFilter()
pCutterSurface.SetInputConnection(pcut.GetOutputPort())
pCutterSurface.MergingOff()

pCutterMapper = vtkPolyDataMapper()
pCutterMapper.SetInputConnection(pCutterSurface.GetOutputPort())

pCutterActor = vtkActor()
pCutterActor.SetMapper(pCutterMapper)
pCutterActor.GetProperty().SetColor(1,1,1)

# Throw in an outline
pOutline = vtkOutlineFilter()
pOutline.SetInputConnection(sample.GetOutputPort())

pOutlineMapper = vtkPolyDataMapper()
pOutlineMapper.SetInputConnection(pOutline.GetOutputPort())

pOutlineActor = vtkActor()
pOutlineActor.SetMapper(pOutlineMapper)

# Now create the crinkle cutter - remove unused points + point data
pcut2 = vtk3DLinearGridCrinkleExtractor()
pcut2.SetInputConnection(extract.GetOutputPort())
pcut2.SetImplicitFunction(plane)
pcut2.RemoveUnusedPointsOn()
pcut2.CopyPointDataOn()
pcut2.CopyCellDataOn()

pCutterSurface2 = vtkGeometryFilter()
pCutterSurface2.SetInputConnection(pcut2.GetOutputPort())
pCutterSurface2.MergingOff()

pCutterMapper2 = vtkPolyDataMapper()
pCutterMapper2.SetInputConnection(pCutterSurface2.GetOutputPort())

pCutterActor2 = vtkActor()
pCutterActor2.SetMapper(pCutterMapper2)
pCutterActor2.GetProperty().SetColor(1,1,1)

# Throw in an outline
pOutline2 = vtkOutlineFilter()
pOutline2.SetInputConnection(sample.GetOutputPort())

pOutlineMapper2 = vtkPolyDataMapper()
pOutlineMapper2.SetInputConnection(pOutline2.GetOutputPort())

pOutlineActor2 = vtkActor()
pOutlineActor2.SetMapper(pOutlineMapper2)

# Time the execution of the usual crinkle cutter
cutter_timer = vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("Standard Crinkle (vtkExtractGeometry):", CT)

# Time the execution of the faster crinkle cutter
cutter_timer.SetFilter(pcut)
pcut.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("vtk3DLinearGridCrinkleExtractor:", CT)
print("\tNumber of threads used: {0}".format(pcut.GetNumberOfThreadsUsed()))

# Time the execution of the faster crinkle cutter
cutter_timer.SetFilter(pcut2)
pcut2.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print ("vtk3DLinearGridCrinkleExtractor (unused points removed):", CT)
print("\tNumber of threads used: {0}".format(pcut.GetNumberOfThreadsUsed()))

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(pOutlineActor)
ren1.AddActor(pCutterActor)
ren2.AddActor(pOutlineActor2)
ren2.AddActor(pCutterActor2)

ren0.SetBackground(0,0,0)
ren1.SetBackground(0,0,0)
ren2.SetBackground(0,0,0)

ren0.SetViewport(0,0,0.333,1);
ren1.SetViewport(0.333,0,0.667,1);
ren2.SetViewport(0.667,0,1,1);
renWin.SetSize(600,200)
ren0.ResetCamera()

cam = ren0.GetActiveCamera()
ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
