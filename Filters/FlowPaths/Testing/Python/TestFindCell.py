#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkCommonDataModel import (
    vtkClosestNPointsStrategy,
    vtkImageData,
)
from vtkmodules.vtkCommonMath import vtkRungeKutta4
from vtkmodules.vtkFiltersCore import vtkAppendFilter
from vtkmodules.vtkFiltersFlowPaths import (
    vtkCompositeInterpolatedVelocityField,
    vtkStreamTracer,
)
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkLineSource
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

# Parameters for controlling the test size
dim = 40
numStreamlines = 50

# Create three volumes of different resolution and butt them together.  This
# tests vtkPointSet::FindCell() on incompatible meshes (hanging and
# duplicate nodes).

spacing = 1.0 / (2.0*dim - 1.0)
v1 = vtkImageData()
v1.SetOrigin(0.0,0,0)
v1.SetDimensions(2*dim,2*dim,2*dim)
v1.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (dim - 1.0)
v2 = vtkImageData()
v2.SetOrigin(1.0,0,0)
v2.SetDimensions(dim,dim,dim)
v2.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (2.0*dim - 1.0)
v3 = vtkImageData()
v3.SetOrigin(2.0,0,0)
v3.SetDimensions(2*dim,2*dim,2*dim)
v3.SetSpacing(spacing,spacing,spacing)

# Append the volumes together to create an unstructured grid with duplicate
# and hanging points.
append = vtkAppendFilter()
append.AddInputData(v1)
append.AddInputData(v2)
append.AddInputData(v3)
append.MergePointsOff()
append.Update()

# Create a uniform vector field in the x-direction
numPts = append.GetOutput().GetNumberOfPoints()
vectors = vtkFloatArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(numPts);
for i in range(0,numPts):
    vectors.SetTuple3(i, 1,0,0)

# A hack but it works
append.GetOutput().GetPointData().SetVectors(vectors)

# Outline around appended unstructured grid
outline = vtkOutlineFilter()
outline.SetInputConnection(append.GetOutputPort())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create streamlines from a rake of seed points
pt1 = [0.001,0.1,0.5]
pt2 = [0.001,0.9,0.5]
line = vtkLineSource()
line.SetResolution(numStreamlines-1)
line.SetPoint1(pt1)
line.SetPoint2(pt2)
line.Update()

rk4 = vtkRungeKutta4()
strategy = vtkClosestNPointsStrategy()
ivp = vtkCompositeInterpolatedVelocityField()
ivp.SetFindCellStrategy(strategy)

streamer = vtkStreamTracer()
streamer.SetInputConnection(append.GetOutputPort())
streamer.SetSourceConnection(line.GetOutputPort())
streamer.SetMaximumPropagation(10)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetMinimumIntegrationStep(0.01)
streamer.SetMaximumIntegrationStep(0.5)
streamer.SetTerminalSpeed(1.0e-12)
streamer.SetMaximumError(1.0e-06)
streamer.SetComputeVorticity(0)
streamer.SetIntegrator(rk4)
streamer.SetInterpolatorPrototype(ivp)
streamer.Update()
reasons = streamer.GetOutput().GetCellData().GetArray("ReasonForTermination")
print(reasons.GetValue(0))

strMapper = vtkPolyDataMapper()
strMapper.SetInputConnection(streamer.GetOutputPort())

strActor = vtkActor()
strActor.SetMapper(strMapper)

# rendering
ren = vtkRenderer()
renWin = vtkRenderWindow()
iren = vtkRenderWindowInteractor()
renWin.AddRenderer(ren)
iren.SetRenderWindow(renWin)

ren.AddActor(outlineActor)
ren.AddActor(strActor)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2)
renWin.SetSize(600, 300)
renWin.Render()

iren.Start()
