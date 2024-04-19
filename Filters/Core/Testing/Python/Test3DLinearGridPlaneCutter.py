# !/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkPlane,
    vtkSphere,
)
from vtkmodules.vtkFiltersCore import (
    vtk3DLinearGridPlaneCutter,
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
    vtkCompositePolyDataMapper,
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
# res = 200

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

# Now create the usual cutter - without a tree
cutter = vtkCutter()
cutter.SetInputConnection(extract.GetOutputPort())
cutter.SetCutFunction(plane)

cutterMapper = vtkCompositePolyDataMapper()
cutterMapper.SetInputConnection(cutter.GetOutputPort())

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

# Now create the faster plane cutter
pcut = vtkPlaneCutter()
pcut.SetInputConnection(extract.GetOutputPort())
pcut.SetPlane(plane)
pcut.BuildTreeOff()

pCutterMapper = vtkPolyDataMapper()
pCutterMapper.SetInputConnection(pcut.GetOutputPort())

pCutterActor = vtkActor()
pCutterActor.SetMapper(pCutterMapper)
pCutterActor.GetProperty().SetColor(1, 1, 1)

# Throw in an outline
pOutline = vtkOutlineFilter()
pOutline.SetInputConnection(sample.GetOutputPort())

pOutlineMapper = vtkPolyDataMapper()
pOutlineMapper.SetInputConnection(outline.GetOutputPort())

pOutlineActor = vtkActor()
pOutlineActor.SetMapper(pOutlineMapper)

# Specialized cutter for 3D linear grids
scut = vtk3DLinearGridPlaneCutter()
scut.SetInputConnection(extract.GetOutputPort())
scut.SetPlane(plane)
scut.ComputeNormalsOff()
scut.MergePointsOff()
scut.InterpolateAttributesOn()

sCutterMapper = vtkPolyDataMapper()
sCutterMapper.SetInputConnection(scut.GetOutputPort())

sCutterActor = vtkActor()
sCutterActor.SetMapper(sCutterMapper)
sCutterActor.GetProperty().SetColor(1, 1, 1)

# Outline
sOutline = vtkOutlineFilter()
sOutline.SetInputConnection(sample.GetOutputPort())

sOutlineMapper = vtkPolyDataMapper()
sOutlineMapper.SetInputConnection(sOutline.GetOutputPort())

sOutlineActor = vtkActor()
sOutlineActor.SetMapper(sOutlineMapper)

# Time the execution of the usual cutter
cutter_timer = vtkExecutionTimer()
cutter_timer.SetFilter(cutter)
cutter.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print("vtkCutter:", CT)

# Time the execution of the usual cutter
cutter_timer.SetFilter(pcut)
pcut.Update()
CT = cutter_timer.GetElapsedWallClockTime()
print("vtkPlaneCutter:", CT)

# Time the execution of the filter w/o sphere tree
cutter_timer.SetFilter(scut)
scut.Update()
SC = cutter_timer.GetElapsedWallClockTime()
print("vtk3DLinearGridPlaneCutter: ", SC)

# Add the actors to the renderer, set the background and size
ren0.AddActor(outlineActor)
ren0.AddActor(cutterActor)
ren1.AddActor(pOutlineActor)
ren1.AddActor(pCutterActor)
ren2.AddActor(sOutlineActor)
ren2.AddActor(sCutterActor)

ren0.SetBackground(0, 0, 0)
ren1.SetBackground(0, 0, 0)
ren2.SetBackground(0, 0, 0)
ren0.SetViewport(0, 0, 0.33, 1)
ren1.SetViewport(0.33, 0, 0.67, 1)
ren2.SetViewport(0.67, 0, 1, 1)
renWin.SetSize(400, 200)
ren0.ResetCamera()

cam = ren0.GetActiveCamera()
ren1.SetActiveCamera(cam)
ren2.SetActiveCamera(cam)
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
