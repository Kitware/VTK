#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
    vtkUnsignedCharArray,
)
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import (
    vtkBoundingBox,
    vtkPolyData,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkFlyingEdges2D,
    vtkGlyph3D,
)
from vtkmodules.vtkFiltersMeshing import vtkVoronoi2D
from vtkmodules.vtkFiltersSources import (
    vtkGlyphSource2D,
    vtkSphereSource,
)
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPointGaussianMapper,
    vtkPointPicker,
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

# Control problem size and set debugging parameters
NPts = 1000
PointsPerBucket = 2
PointOfInterest = 6
GenerateFlower = 1

# create some points and display them
#
math = vtkMath()
math.RandomSeed(31415)
points = vtkPoints()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)

ptMapper = vtkPointGaussianMapper()
ptMapper.SetInputData(profile)
ptMapper.EmissiveOff()
ptMapper.SetScaleFactor(0.0)

ptActor = vtkActor()
ptActor.SetMapper(ptMapper)
ptActor.GetProperty().SetColor(0,0,0)
ptActor.GetProperty().SetPointSize(2)

# Tessellate them
#
voronoi = vtkVoronoi2D()
voronoi.SetInputData(profile)
voronoi.SetOutputTypeToVoronoi()
voronoi.SetGenerateCellScalarsToNone()
voronoi.SetPointOfInterest(PointOfInterest)
voronoi.GetLocator().SetNumberOfPointsPerBucket(PointsPerBucket)
voronoi.SetGenerateVoronoiFlower(GenerateFlower)

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
voronoi.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Number of points processed: {0}".format(NPts))
print("   Time to generate Voronoi tessellation: {0}".format(time))
print("   Number of threads used: {0}".format(voronoi.GetNumberOfThreadsUsed()))

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(voronoi.GetOutputPort())
if voronoi.GetGenerateCellScalars() == 1:
    mapper.SetScalarRange(0,NPts)
elif voronoi.GetGenerateCellScalars() == 2:
    mapper.SetScalarRange(0,voronoi.GetNumberOfThreadsUsed())
else:
    mapper.ScalarVisibilityOff()
print("Scalar Range: {}".format(mapper.GetScalarRange()))

nc = vtkNamedColors()
c = [0,0,0,0]

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(nc.GetColor3d("slate_grey"))
actor.GetProperty().EdgeVisibilityOn()
actor.SetPosition(0,0,-0.001)

# Draw sphere at point of interest
sphere = vtkSphereSource()
sphere.SetRadius(0.001)
sphere.SetThetaResolution(16)
sphere.SetPhiResolution(8)
if PointOfInterest >= 0:
    sphere.SetCenter(points.GetPoint(PointOfInterest))

sphereMapper = vtkPolyDataMapper()
sphereMapper.SetInputConnection(sphere.GetOutputPort())

sphereActor = vtkActor()
sphereActor.SetMapper(sphereMapper)
sphereActor.GetProperty().SetColor(0,0,0)

# Voronoi flower represented by sampled points
fMapper = vtkPointGaussianMapper()
fMapper.SetInputConnection(voronoi.GetOutputPort(2))
fMapper.EmissiveOff()
fMapper.SetScaleFactor(0.0)

fActor = vtkActor()
fActor.SetMapper(fMapper)
fActor.GetProperty().SetPointSize(2)
fActor.GetProperty().SetColor(0,0,1)

# Voronoi flower circles
circle = vtkGlyphSource2D()
circle.SetResolution(64)
circle.SetGlyphTypeToCircle()

cGlyph = vtkGlyph3D()
cGlyph.SetInputConnection(voronoi.GetOutputPort(3))
cGlyph.SetSourceConnection(circle.GetOutputPort())
cGlyph.SetScaleModeToScaleByScalar()
cGlyph.SetScaleFactor(2)
cGlyph.Update()

circleColors = vtkUnsignedCharArray()
circleColors.SetNumberOfComponents(4)
circleColors.SetNumberOfTuples(5)
nc.GetColor("Blue",c)
circleColors.SetTuple4(0,c[0],c[1],c[2],c[3])
nc.GetColor("Tomato",c)
circleColors.SetTuple4(1,c[0],c[1],c[2],c[3])
nc.GetColor("Wheat",c)
circleColors.SetTuple4(2,c[0],c[1],c[2],c[3])
nc.GetColor("lavender",c)
circleColors.SetTuple4(3,c[0],c[1],c[2],c[3])
nc.GetColor("Mint",c)
circleColors.SetTuple4(4,c[0],c[1],c[2],c[3])

cGlyphWithColors = cGlyph.GetOutput()
cGlyphWithColors.GetCellData().SetScalars(circleColors)

cMapper = vtkPolyDataMapper()
cMapper.SetInputData(cGlyphWithColors)
cMapper.SetScalarModeToUseCellData()

cActor = vtkActor()
cActor.SetMapper(cMapper)
cActor.GetProperty().SetColor(0,0,0)
cActor.GetProperty().EdgeVisibilityOn()
cActor.GetProperty().SetLineWidth(3)
cActor.GetProperty().SetOpacity(0.35)

# Implicit function. This basically generates a
# silhouette outline of the Voronoi flower.
bounds = [0,0,0,0,0,0]
bbox = vtkBoundingBox()
bbox.SetBounds(voronoi.GetOutput(2).GetBounds())
bbox.ScaleAboutCenter(1.10)
bbox.GetBounds(bounds)

sample = vtkSampleFunction()
sample.SetSampleDimensions(256,256,1)
sample.SetModelBounds(bounds)
sample.SetImplicitFunction(voronoi.GetSpheres())
sample.Update()
print(sample.GetOutput().GetPointData().GetScalars().GetRange())

contour = vtkFlyingEdges2D()
contour.SetInputConnection(sample.GetOutputPort())
contour.SetValue(0,0.0)

iMapper = vtkPolyDataMapper()
iMapper.SetInputConnection(contour.GetOutputPort())
iMapper.ScalarVisibilityOff()

iActor = vtkActor()
iActor.SetMapper(iMapper)
iActor.GetProperty().SetColor(0,0,0)

# Create the RenderWindow, Renderer and both Actors
#
ren0 = vtkRenderer()
ren0.SetViewport(0,0,0.5,1)
ren1 = vtkRenderer()
ren1.SetViewport(0.5,0,1,1)
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
if PointOfInterest >= 0:
    ren0.AddActor(sphereActor)
    ren1.AddActor(sphereActor)
    if GenerateFlower > 0:
         ren1.AddActor(cActor)
         ren0.AddActor(fActor)
ren0.AddActor(actor)
ren1.AddActor(actor)

ren0.SetBackground(1,1,1)
renWin.SetSize(600,300)
renWin.Render()
cam1 = ren0.GetActiveCamera()
if PointOfInterest >= 0:
    xyz = []
    xyz = points.GetPoint(PointOfInterest)
    cam1.SetFocalPoint(xyz)
    cam1.SetPosition(xyz[0],xyz[1],xyz[2]+1)
    renWin.Render()
    ren0.ResetCameraClippingRange()
    cam1.Zoom(3.25)

ren1.SetBackground(1,1,1)
ren1.SetActiveCamera(ren0.GetActiveCamera())
ren1.AddActor(iActor)

# added these unused default arguments so that the prototype
# matches as required in python.
def reportPointId(obj=None, event=""):
    print("Point Id: {}".format(picker.GetPointId()))

picker = vtkPointPicker()
picker.AddObserver("EndPickEvent",reportPointId)
picker.PickFromListOn()
picker.AddPickList(ptActor)
iren.SetPicker(picker)

iren.Initialize()
# --- end of script --
