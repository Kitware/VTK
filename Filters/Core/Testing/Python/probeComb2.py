#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import (
    vtkCellLocatorStrategy,
    vtkStaticCellLocator,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkProbeFilter,
    vtkStructuredGridOutlineFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkTransformPolyDataFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkPlaneSource
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

# Test alternative methods of probing data including
# using vtkFindCellStrategy and directly specifying
# a cell locator.

# Control test size
res = 50

# create planes
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkMultiBlockPLOT3DReader()
pl3d.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
pl3d.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
output = pl3d.GetOutput().GetBlock(0)

# Probe with three separate planes. Use different probing approaches on each
# of the three planes (vtkDataSet::FindCell(), directly specifying a cell
# locator, and using a vtkFindCellStrategy. Then isocontour the planes.

# First plane
plane = vtkPlaneSource()
plane.SetResolution(res,res)
transP1 = vtkTransform()
transP1.Translate(3.7,0.0,28.37)
transP1.Scale(5,5,5)
transP1.RotateY(90)
tpd1 = vtkTransformPolyDataFilter()
tpd1.SetInputConnection(plane.GetOutputPort())
tpd1.SetTransform(transP1)
probe1 = vtkProbeFilter()
probe1.SetInputConnection(tpd1.GetOutputPort())
probe1.SetSourceData(output)
probe1.DebugOn()
contour1 = vtkContourFilter()
contour1.SetInputConnection(probe1.GetOutputPort())
contour1.GenerateValues(50,output.GetScalarRange())
mapProbe1 = vtkPolyDataMapper()
mapProbe1.SetInputConnection(contour1.GetOutputPort())
mapProbe1.SetScalarRange(output.GetScalarRange())
probe1Actor = vtkActor()
probe1Actor.SetMapper(mapProbe1)

outTpd1 = vtkOutlineFilter()
outTpd1.SetInputConnection(tpd1.GetOutputPort())
mapTpd1 = vtkPolyDataMapper()
mapTpd1.SetInputConnection(outTpd1.GetOutputPort())
tpd1Actor = vtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0,0,0)

# Next plane
transP2 = vtkTransform()
transP2.Translate(9.2,0.0,31.20)
transP2.Scale(5,5,5)
transP2.RotateY(90)
tpd2 = vtkTransformPolyDataFilter()
tpd2.SetInputConnection(plane.GetOutputPort())
tpd2.SetTransform(transP2)
cellLoc = vtkStaticCellLocator()
probe2 = vtkProbeFilter()
probe2.SetInputConnection(tpd2.GetOutputPort())
probe2.SetSourceData(output)
probe2.SetCellLocatorPrototype(cellLoc)
probe2.DebugOn()
contour2 = vtkContourFilter()
contour2.SetInputConnection(probe2.GetOutputPort())
contour2.GenerateValues(50,output.GetScalarRange())
mapProbe2 = vtkPolyDataMapper()
mapProbe2.SetInputConnection(contour2.GetOutputPort())
mapProbe2.SetScalarRange(output.GetScalarRange())
probe2Actor = vtkActor()
probe2Actor.SetMapper(mapProbe2)

outTpd2 = vtkOutlineFilter()
outTpd2.SetInputConnection(tpd2.GetOutputPort())
mapTpd2 = vtkPolyDataMapper()
mapTpd2.SetInputConnection(outTpd2.GetOutputPort())
tpd2Actor = vtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0,0,0)

# Third plane
transP3 = vtkTransform()
transP3.Translate(13.27,0.0,33.30)
transP3.Scale(5,5,5)
transP3.RotateY(90)
tpd3 = vtkTransformPolyDataFilter()
tpd3.SetInputConnection(plane.GetOutputPort())
tpd3.SetTransform(transP3)
strategy = vtkCellLocatorStrategy()
probe3 = vtkProbeFilter()
probe3.SetInputConnection(tpd3.GetOutputPort())
probe3.SetSourceData(output)
probe3.SetFindCellStrategy(strategy)
probe3.DebugOn()
contour3 = vtkContourFilter()
contour3.SetInputConnection(probe3.GetOutputPort())
contour3.GenerateValues(50,output.GetScalarRange())
mapProbe3 = vtkPolyDataMapper()
mapProbe3.SetInputConnection(contour3.GetOutputPort())
mapProbe3.SetScalarRange(output.GetScalarRange())
probe3Actor = vtkActor()
probe3Actor.SetMapper(mapProbe3)

outTpd3 = vtkOutlineFilter()
outTpd3.SetInputConnection(tpd3.GetOutputPort())
mapTpd3 = vtkPolyDataMapper()
mapTpd3.SetInputConnection(outTpd3.GetOutputPort())
tpd3Actor = vtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0,0,0)

# Create an outline around the structured grid
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)

# Add the actors
ren1.AddActor(outlineActor)
ren1.AddActor(probe1Actor)
ren1.AddActor(probe2Actor)
ren1.AddActor(probe3Actor)
ren1.AddActor(tpd1Actor)
ren1.AddActor(tpd2Actor)
ren1.AddActor(tpd3Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)

# Setup a camera view
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)

iren.Initialize()
renWin.Render()
iren.Start()
# --- end of script --
