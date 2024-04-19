#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkPoints,
    vtkRandomPool,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersPoints import vtkExtractEnclosedPoints
from vtkmodules.vtkFiltersSources import vtkSphereSource
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
numPts = 500

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Create an enclosing surface
ss = vtkSphereSource()
ss.SetPhiResolution(25)
ss.SetThetaResolution(38)
ss.SetCenter(4.5, 5.5, 5.0)
ss.SetRadius(2.5)

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(ss.GetOutputPort())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetRepresentationToWireframe()

# Generate some random points and scalars
points = vtkPoints()
points.SetNumberOfPoints(numPts);

da = points.GetData()
pool = vtkRandomPool()
pool.PopulateDataArray(da, 0, 2.25, 7)
pool.PopulateDataArray(da, 1, 1, 10)
pool.PopulateDataArray(da, 2, 0.5, 10.5)

scalars = vtkFloatArray()
scalars.SetNumberOfTuples(numPts)
scalars.SetName("Random Scalars");
pool.PopulateDataArray(scalars, 100,200)

profile = vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars);

extract = vtkExtractEnclosedPoints()
extract.SetInputData(profile)
extract.SetSurfaceConnection(ss.GetOutputPort())

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
extract.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to extract points: {0}".format(time))

glyph = vtkSphereSource()
glypher = vtkGlyph3D()
glypher.SetInputConnection(extract.GetOutputPort())
#glypher.SetInputConnection(extract.GetOutputPort(1))
glypher.SetSourceConnection(glyph.GetOutputPort())
glypher.SetScaleModeToDataScalingOff()
glypher.SetScaleFactor(0.25)

pointsMapper = vtkPolyDataMapper()
pointsMapper.SetInputConnection(glypher.GetOutputPort())
pointsMapper.SetScalarRange(100,200)

pointsActor = vtkActor()
pointsActor.SetMapper(pointsMapper)

# Add actors
ren.AddActor(actor)
ren.AddActor(pointsActor)

# Standard testing code.
renWin.SetSize(300,300)
renWin.Render()

# render the image
#
renWin.Render()
iren.Start()
