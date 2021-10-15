#!/usr/bin/env python
import vtk

# Test the processing of point and cell data with vtkCookieCutter
#

# Control the size of the test
res = 101
cylRes = 20

# Control the coloring of the data
coloring = 1

# The orientation of the plane
normal = [0.1,1,0.8]

# Create a pipeline that cuts a volume to create a plane. This plane will be
# cookie cut with a cylinder.

# The cylinder is cut to produce a trim loop.  This trim loop will cookie cut
# the plane mentioned previously.

# Along the way, various combinations of the cell data
# and point data will be created which are processed by
# the cookie cutter.

# Create a synthetic source: sample a sphere across a volume
cyl = vtk.vtkCylinder()
cyl.SetCenter( 0.0,0.0,0.0)
cyl.SetRadius(0.25)
cyl.SetAxis(0,1,0)

sample = vtk.vtkSampleFunction()
sample.SetImplicitFunction(cyl)
sample.SetModelBounds(-0.75,0.75, -1,1, -0.5,0.5)
sample.SetSampleDimensions(res,res,res)
sample.ComputeNormalsOff()
sample.SetOutputScalarTypeToFloat()
sample.Update()

# The cut plane
plane = vtk.vtkPlane()
plane.SetOrigin(0,0,0)
plane.SetNormal(normal)

# Cut the volume quickly
cut = vtk.vtkFlyingEdgesPlaneCutter()
cut.SetInputConnection(sample.GetOutputPort())
cut.SetPlane(plane)
cut.ComputeNormalsOff()
cut.Update()

# Create cell data
pd2cd = vtk.vtkPointDataToCellData()
pd2cd.SetInputConnection(cut.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.Update()

cutMapper = vtk.vtkPolyDataMapper()
cutMapper.SetInputConnection(pd2cd.GetOutputPort())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper);
cutActor.GetProperty().SetColor(0.1,0.1,0.1)
cutActor.GetProperty().SetRepresentationToWireframe()

# Clip a cylinder shell to produce a a trim loop.
shell = vtk.vtkCylinderSource()
shell.SetCenter(0,0,0)
shell.SetResolution(cylRes)
shell.SetHeight(5)
shell.CappingOff()
shell.Update()

clippedShell = vtk.vtkPolyDataPlaneClipper()
clippedShell.SetInputConnection(shell.GetOutputPort())
clippedShell.SetPlane(plane)
clippedShell.ClippingLoopsOn()
clippedShell.CappingOff()
clippedShell.Update()

sampleImp = vtk.vtkSampleImplicitFunctionFilter()
sampleImp.SetInputConnection(clippedShell.GetOutputPort(1))
sampleImp.SetImplicitFunction(plane)
sampleImp.ComputeGradientsOff()
sampleImp.SetScalarArrayName("scalars")
sampleImp.Update()

clippedShellMapper = vtk.vtkPolyDataMapper()
clippedShellMapper.SetInputConnection(sampleImp.GetOutputPort())
clippedShellMapper.ScalarVisibilityOff()

clippedShellActor = vtk.vtkActor()
clippedShellActor.SetMapper(clippedShellMapper);

trimMapper = vtk.vtkPolyDataMapper()
trimMapper.SetInputConnection(sampleImp.GetOutputPort(0))
trimMapper.ScalarVisibilityOff()

trimActor = vtk.vtkActor()
trimActor.SetMapper(trimMapper);
trimActor.GetProperty().SetColor(0,0,1)

# Build a loop from the clipper
buildLoops = vtk.vtkContourLoopExtraction()
buildLoops.SetInputConnection(sampleImp.GetOutputPort(0))
buildLoops.Update()

# Test cell data
cookie0 = vtk.vtkCookieCutter()
cookie0.SetInputConnection(pd2cd.GetOutputPort())
cookie0.SetLoopsConnection(buildLoops.GetOutputPort())
if coloring == 0:
    cookie0.PassCellDataOff()
    cookie0.PassPointDataOff()
else:
    cookie0.PassCellDataOn()
    cookie0.PassPointDataOff()

timer = vtk.vtkTimerLog()
timer.StartTimer()
cookie0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cookie cutting with cell data: {0}".format(time))

cookie0Mapper = vtk.vtkPolyDataMapper()
cookie0Mapper.SetInputConnection(cookie0.GetOutputPort())
cookie0Mapper.SetScalarRange(0,0.1)

cookie0Actor = vtk.vtkActor()
cookie0Actor.SetMapper(cookie0Mapper);

# Test point data - mesh interpolation
cookie1 = vtk.vtkCookieCutter()
cookie1.SetInputConnection(pd2cd.GetOutputPort())
cookie1.SetLoopsConnection(buildLoops.GetOutputPort())
if coloring == 0:
    cookie1.PassCellDataOff()
    cookie1.PassPointDataOff()
else:
    cookie1.PassCellDataOff()
    cookie1.PassPointDataOn()
    cookie1.SetPointInterpolationToMeshEdges()

timer = vtk.vtkTimerLog()
timer.StartTimer()
cookie1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cookie cutting with mesh edge interpolation: {0}".format(time))

cookie1Mapper = vtk.vtkPolyDataMapper()
cookie1Mapper.SetInputConnection(cookie1.GetOutputPort())
cookie1Mapper.SetScalarRange(0,0.1)
cookie1Mapper.SetScalarModeToUsePointFieldData()
cookie1Mapper.SelectColorArray("scalars")

cookie1Actor = vtk.vtkActor()
cookie1Actor.SetMapper(cookie1Mapper);

# Test point data - loop edge interpolation
cookie2 = vtk.vtkCookieCutter()
cookie2.SetInputConnection(pd2cd.GetOutputPort())
cookie2.SetLoopsConnection(buildLoops.GetOutputPort())
if coloring == 0:
    cookie2.PassCellDataOff()
    cookie2.PassPointDataOff()
else:
    cookie2.PassCellDataOff()
    cookie2.PassPointDataOn()
    cookie2.SetPointInterpolationToLoopEdges()

timer = vtk.vtkTimerLog()
timer.StartTimer()
cookie2.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cookie cutting with loop edge interpolation: {0}".format(time))

cookie2Mapper = vtk.vtkPolyDataMapper()
cookie2Mapper.SetInputConnection(cookie2.GetOutputPort())
cookie2Mapper.SetScalarRange(0,0.1)
cookie2Mapper.SetScalarModeToUsePointFieldData()
cookie2Mapper.SelectColorArray("scalars")

cookie2Actor = vtk.vtkActor()
cookie2Actor.SetMapper(cookie2Mapper);

# Bounding boxes are always nice
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(sample.GetOutputPort())
outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Create three renderers, one to display cell data,
# one to display mesh-interpolated point data, one
# to display trim loop interpolated point data.
ren0 = vtk.vtkRenderer()
ren0.GetActiveCamera().SetPosition(normal)
ren0.SetViewport(0,0,0.333,1.0)
ren1 = vtk.vtkRenderer()
ren1.SetActiveCamera(ren0.GetActiveCamera())
ren1.SetViewport(0.333,0,0.667,1.0)
ren2 = vtk.vtkRenderer()
ren2.SetActiveCamera(ren0.GetActiveCamera())
ren2.SetViewport(0.667,0,1.0,1.0)

renWin = vtk.vtkRenderWindow()
renWin.AddRenderer( ren0 )
renWin.AddRenderer( ren1 )
renWin.AddRenderer( ren2 )
renWin.SetSize(600,200)

#ren0.AddActor(cutActor)
#ren0.AddActor(trimActor)
#ren0.AddActor(clippedShellActor)
#ren0.AddActor(cookie0Actor)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren0.AddActor(outlineActor)
ren0.AddActor(cookie0Actor)

ren1.AddActor(outlineActor)
ren1.AddActor(cookie1Actor)

ren2.AddActor(outlineActor)
ren2.AddActor(cookie2Actor)

ren0.ResetCamera()

renWin.Render()
iren.Start()
