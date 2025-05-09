#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import vtkHyperStreamline
from vtkmodules.vtkFiltersGeometry import vtkImageDataGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkImagingHybrid import vtkPointLoad
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkLogLookupTable,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingAnnotation import vtkScalarBarActor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the scalar bar actor using a logarithmic lookup table
#
VTK_INTEGRATE_BOTH_DIRECTIONS = 2
#
# generate tensors
ptLoad = vtkPointLoad()
ptLoad.SetLoadValue(100.0)
ptLoad.SetSampleDimensions(20,20,20)
ptLoad.ComputeEffectiveStressOn()
ptLoad.SetModelBounds(-10,10,-10,10,-10,10)
# Generate hyperstreamlines
s1 = vtkHyperStreamline()
s1.SetInputConnection(ptLoad.GetOutputPort())
s1.SetStartPosition(9,9,-9)
s1.IntegrateMinorEigenvector()
s1.SetMaximumPropagationDistance(18.0)
s1.SetIntegrationStepLength(0.1)
s1.SetStepLength(0.01)
s1.SetRadius(0.25)
s1.SetNumberOfSides(18)
s1.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s1.Update()
# Map hyperstreamlines
lut = vtkLogLookupTable()
lut.SetHueRange(.6667,0.0)
scalarBar = vtkScalarBarActor()
scalarBar.SetLookupTable(lut)
scalarBar.SetTitle("Stress")
scalarBar.GetPositionCoordinate().SetCoordinateSystemToNormalizedViewport()
scalarBar.GetPositionCoordinate().SetValue(0.1,0.05)
scalarBar.SetOrientationToVertical()
scalarBar.SetWidth(0.1)
scalarBar.SetHeight(0.9)
scalarBar.SetPosition(0.01,0.1)
scalarBar.SetLabelFormat("%-#6.3f")
scalarBar.GetLabelTextProperty().SetColor(1,0,0)
scalarBar.GetTitleTextProperty().SetColor(1,0,0)
s1Mapper = vtkPolyDataMapper()
s1Mapper.SetInputConnection(s1.GetOutputPort())
s1Mapper.SetLookupTable(lut)
ptLoad.Update()
#force update for scalar range
s1Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s1Actor = vtkActor()
s1Actor.SetMapper(s1Mapper)
s2 = vtkHyperStreamline()
s2.SetInputConnection(ptLoad.GetOutputPort())
s2.SetStartPosition(-9,-9,-9)
s2.IntegrateMinorEigenvector()
s2.SetMaximumPropagationDistance(18.0)
s2.SetIntegrationStepLength(0.1)
s2.SetStepLength(0.01)
s2.SetRadius(0.25)
s2.SetNumberOfSides(18)
s2.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s2.Update()
s2Mapper = vtkPolyDataMapper()
s2Mapper.SetInputConnection(s2.GetOutputPort())
s2Mapper.SetLookupTable(lut)
s2Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s2Actor = vtkActor()
s2Actor.SetMapper(s2Mapper)
s3 = vtkHyperStreamline()
s3.SetInputConnection(ptLoad.GetOutputPort())
s3.SetStartPosition(9,-9,-9)
s3.IntegrateMinorEigenvector()
s3.SetMaximumPropagationDistance(18.0)
s3.SetIntegrationStepLength(0.1)
s3.SetStepLength(0.01)
s3.SetRadius(0.25)
s3.SetNumberOfSides(18)
s3.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s3.Update()
s3Mapper = vtkPolyDataMapper()
s3Mapper.SetInputConnection(s3.GetOutputPort())
s3Mapper.SetLookupTable(lut)
s3Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s3Actor = vtkActor()
s3Actor.SetMapper(s3Mapper)
s4 = vtkHyperStreamline()
s4.SetInputConnection(ptLoad.GetOutputPort())
s4.SetStartPosition(-9,9,-9)
s4.IntegrateMinorEigenvector()
s4.SetMaximumPropagationDistance(18.0)
s4.SetIntegrationStepLength(0.1)
s4.SetStepLength(0.01)
s4.SetRadius(0.25)
s4.SetNumberOfSides(18)
s4.SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS)
s4.Update()
s4Mapper = vtkPolyDataMapper()
s4Mapper.SetInputConnection(s4.GetOutputPort())
s4Mapper.SetLookupTable(lut)
s4Mapper.SetScalarRange(ptLoad.GetOutput().GetScalarRange())
s4Actor = vtkActor()
s4Actor.SetMapper(s4Mapper)
# plane for context
#
g = vtkImageDataGeometryFilter()
g.SetInputConnection(ptLoad.GetOutputPort())
g.SetExtent(0,100,0,100,0,0)
g.Update()
#for scalar range
gm = vtkPolyDataMapper()
gm.SetInputConnection(g.GetOutputPort())
gm.SetScalarRange(g.GetOutput().GetScalarRange())
ga = vtkActor()
ga.SetMapper(gm)
# Create outline around data
#
outline = vtkOutlineFilter()
outline.SetInputConnection(ptLoad.GetOutputPort())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
# Create cone indicating application of load
#
coneSrc = vtkConeSource()
coneSrc.SetRadius(.5)
coneSrc.SetHeight(2)
coneMap = vtkPolyDataMapper()
coneMap.SetInputConnection(coneSrc.GetOutputPort())
coneActor = vtkActor()
coneActor.SetMapper(coneMap)
coneActor.SetPosition(0,0,11)
coneActor.RotateY(90)
coneActor.GetProperty().SetColor(1,0,0)
# Create the rendering infrastructure
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
camera = vtkCamera()
camera.SetFocalPoint(0.113766,-1.13665,-1.01919)
camera.SetPosition(-29.4886,-63.1488,26.5807)
camera.SetViewAngle(24.4617)
camera.SetViewUp(0.17138,0.331163,0.927879)
camera.SetClippingRange(1,100)
ren1.AddViewProp(scalarBar)
ren1.AddActor(s1Actor)
ren1.AddActor(s2Actor)
ren1.AddActor(s3Actor)
ren1.AddActor(s4Actor)
ren1.AddActor(outlineActor)
ren1.AddActor(coneActor)
ren1.AddActor(ga)
ren1.SetBackground(1.0,1.0,1.0)
ren1.SetActiveCamera(camera)
renWin.SetSize(300,300)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
