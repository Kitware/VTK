#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCylinder,
    vtkPlane,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkFlyingEdgesPlaneCutter,
    vtkPointDataToCellData,
    vtkPolyDataPlaneClipper,
)
from vtkmodules.vtkFiltersGeneral import vtkSampleImplicitFunctionFilter
from vtkmodules.vtkFiltersModeling import (
    vtkContourLoopExtraction,
    vtkImprintFilter,
    vtkOutlineFilter,
)
from vtkmodules.vtkFiltersSources import vtkCylinderSource
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

# Test the processing of point and cell data with vtkImprintFilter
#

# Control the size of the test
res = 101
cylRes = 20

# Control the coloring of the data
coloring = 1

# The orientation of the plane
normal = [0,1.01,1]

# Create a pipeline that cuts a volume to create a plane. This plane will be
# cookie cut with a cylinder.

# The cylinder is cut to produce a trim loop.  This trim loop will cookie cut
# the plane mentioned previously.

# Along the way, various combinations of the cell data
# and point data will be created which are processed by
# the cookie cutter.

# Create a synthetic source: sample a sphere across a volume
cyl = vtkCylinder()
cyl.SetCenter( 0.0,0.0,0.0)
cyl.SetRadius(0.25)
cyl.SetAxis(0,1,0)

sample = vtkSampleFunction()
sample.SetImplicitFunction(cyl)
sample.SetModelBounds(-0.75,0.75, -1,1, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.SetOutputScalarTypeToFloat()
sample.Update()

# The cut plane
plane = vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(normal)

# Cut the volume quickly
cut = vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(sample.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()
cut.Update()

# Create cell data
pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(cut.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.Update()

cutMapper = vtkPolyDataMapper()
cutMapper.SetInputConnection(pd2cd.GetOutputPort())

cutActor = vtkActor()
cutActor.SetMapper(cutMapper);
cutActor.GetProperty().SetColor(0.1,0.1,0.1)
cutActor.GetProperty().SetRepresentationToWireframe()

# Clip a cylinder shell to produce a a trim loop.
shell = vtkCylinderSource()
shell.SetCenter(0,0,0)
shell.SetResolution(cylRes)
shell.SetHeight(5)
shell.CappingOff()
shell.Update()

clippedShell = vtkPolyDataPlaneClipper()
clippedShell.SetInputConnection(shell.GetOutputPort())
clippedShell.SetPlane(plane)
clippedShell.ClippingLoopsOn()
clippedShell.CappingOff()
clippedShell.Update()

sampleImp = vtkSampleImplicitFunctionFilter()
sampleImp.SetInputConnection(clippedShell.GetOutputPort(1))
sampleImp.SetImplicitFunction(plane)
sampleImp.ComputeGradientsOff()
sampleImp.SetScalarArrayName("scalars")
sampleImp.Update()

clippedShellMapper = vtkPolyDataMapper()
clippedShellMapper.SetInputConnection(sampleImp.GetOutputPort())
clippedShellMapper.ScalarVisibilityOff()

clippedShellActor = vtkActor()
clippedShellActor.SetMapper(clippedShellMapper);

trimMapper = vtkPolyDataMapper()
trimMapper.SetInputConnection(sampleImp.GetOutputPort(0))
trimMapper.ScalarVisibilityOff()

trimActor = vtkActor()
trimActor.SetMapper(trimMapper);
trimActor.GetProperty().SetColor(0,0,1)

# Build a loop from the clipper
buildLoops = vtkContourLoopExtraction()
buildLoops.SetInputConnection(sampleImp.GetOutputPort(0))
buildLoops.CleanPointsOn()
buildLoops.Update()

# Test passing cell data
imprint0 = vtkImprintFilter()
imprint0.SetTargetConnection(pd2cd.GetOutputPort())
imprint0.SetImprintConnection(buildLoops.GetOutputPort())
imprint0.SetOutputTypeToImprintedRegion()
imprint0.BoundaryEdgeInsertionOn()
imprint0.SetTolerance(0.0001)
imprint0.SetMergeTolerance(0.0005)
if coloring == 0:
    imprint0.PassCellDataOff()
    imprint0.PassPointDataOff()
else:
    imprint0.PassCellDataOn()
    imprint0.PassPointDataOff()

timer = vtkTimerLog()
timer.StartTimer()
imprint0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Imprinting with cell data: {0}".format(time))

imprint0Mapper = vtkPolyDataMapper()
imprint0Mapper.SetInputConnection(imprint0.GetOutputPort())
imprint0Mapper.SetScalarRange(0,0.1)

imprint0Actor = vtkActor()
imprint0Actor.SetMapper(imprint0Mapper);

# Test point data - mesh interpolation
imprint1 = vtkImprintFilter()
imprint1.SetTargetConnection(pd2cd.GetOutputPort())
imprint1.SetImprintConnection(buildLoops.GetOutputPort())
imprint1.SetOutputTypeToImprintedRegion()
imprint1.BoundaryEdgeInsertionOn()
imprint1.SetTolerance(0.0001)
imprint1.SetMergeTolerance(0.0005)
if coloring == 0:
    imprint1.PassCellDataOff()
    imprint1.PassPointDataOff()
else:
    imprint1.PassCellDataOff()
    imprint1.PassPointDataOn()
    imprint1.SetPointInterpolationToTargetEdges()

timer = vtkTimerLog()
timer.StartTimer()
imprint1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Imprinting with target edge interpolation: {0}".format(time))

imprint1Mapper = vtkPolyDataMapper()
imprint1Mapper.SetInputConnection(imprint1.GetOutputPort())
imprint1Mapper.SetScalarRange(0,0.1)
imprint1Mapper.SetScalarModeToUsePointFieldData()
imprint1Mapper.SelectColorArray("scalars")

imprint1Actor = vtkActor()
imprint1Actor.SetMapper(imprint1Mapper);

# Test point data - loop edge interpolation
imprint2 = vtkImprintFilter()
imprint2.SetTargetConnection(pd2cd.GetOutputPort())
imprint2.SetImprintConnection(buildLoops.GetOutputPort())
imprint2.SetOutputTypeToImprintedRegion()
imprint2.BoundaryEdgeInsertionOn()
imprint2.SetTolerance(0.0001)
imprint2.SetMergeTolerance(0.0005)
if coloring == 0:
    imprint2.PassCellDataOff()
    imprint2.PassPointDataOff()
else:
    imprint2.PassCellDataOff()
    imprint2.PassPointDataOn()
    imprint2.SetPointInterpolationToImprintEdges()

timer = vtkTimerLog()
timer.StartTimer()
imprint2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Imprinting with imprint edge interpolation: {0}".format(time))

imprint2Mapper = vtkPolyDataMapper()
imprint2Mapper.SetInputConnection(imprint2.GetOutputPort())
imprint2Mapper.SetScalarRange(0,0.1)
imprint2Mapper.SetScalarModeToUsePointFieldData()
imprint2Mapper.SelectColorArray("scalars")

imprint2Actor = vtkActor()
imprint2Actor.SetMapper(imprint2Mapper);

# Bounding boxes are always nice
outline = vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create three renderers, one to display cell data,
# one to display mesh-interpolated point data, one
# to display trim loop interpolated point data.
ren0 = vtkRenderer()
ren0.GetActiveCamera().SetPosition(normal)
ren0.SetViewport(0,0,0.333,1.0)
ren1 = vtkRenderer()
ren1.SetActiveCamera(ren0.GetActiveCamera())
ren1.SetViewport(0.333,0,0.667,1.0)
ren2 = vtkRenderer()
ren2.SetActiveCamera(ren0.GetActiveCamera())
ren2.SetViewport(0.667,0,1.0,1.0)

renWin = vtkRenderWindow()
renWin.AddRenderer( ren0 )
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600,200)

#ren0.AddActor(cutActor)
#ren0.AddActor(trimActor)
#ren0.AddActor(clippedShellActor)
#ren0.AddActor(imprint0Actor)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren0.AddActor(outlineActor)
ren0.AddActor(imprint0Actor)

ren1.AddActor(outlineActor)
ren1.AddActor(imprint1Actor)

ren2.AddActor(outlineActor)
ren2.AddActor(imprint2Actor)

ren0.ResetCamera()

renWin.Render()
iren.Start()
