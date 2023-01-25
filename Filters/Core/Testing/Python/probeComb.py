#!/usr/bin/env python
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkAppendPolyData,
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
plane = vtkPlaneSource()
plane.SetResolution(50,50)
transP1 = vtkTransform()
transP1.Translate(3.7,0.0,28.37)
transP1.Scale(5,5,5)
transP1.RotateY(90)
tpd1 = vtkTransformPolyDataFilter()
tpd1.SetInputConnection(plane.GetOutputPort())
tpd1.SetTransform(transP1)
outTpd1 = vtkOutlineFilter()
outTpd1.SetInputConnection(tpd1.GetOutputPort())
mapTpd1 = vtkPolyDataMapper()
mapTpd1.SetInputConnection(outTpd1.GetOutputPort())
tpd1Actor = vtkActor()
tpd1Actor.SetMapper(mapTpd1)
tpd1Actor.GetProperty().SetColor(0,0,0)
transP2 = vtkTransform()
transP2.Translate(9.2,0.0,31.20)
transP2.Scale(5,5,5)
transP2.RotateY(90)
tpd2 = vtkTransformPolyDataFilter()
tpd2.SetInputConnection(plane.GetOutputPort())
tpd2.SetTransform(transP2)
outTpd2 = vtkOutlineFilter()
outTpd2.SetInputConnection(tpd2.GetOutputPort())
mapTpd2 = vtkPolyDataMapper()
mapTpd2.SetInputConnection(outTpd2.GetOutputPort())
tpd2Actor = vtkActor()
tpd2Actor.SetMapper(mapTpd2)
tpd2Actor.GetProperty().SetColor(0,0,0)
transP3 = vtkTransform()
transP3.Translate(13.27,0.0,33.30)
transP3.Scale(5,5,5)
transP3.RotateY(90)
tpd3 = vtkTransformPolyDataFilter()
tpd3.SetInputConnection(plane.GetOutputPort())
tpd3.SetTransform(transP3)
outTpd3 = vtkOutlineFilter()
outTpd3.SetInputConnection(tpd3.GetOutputPort())
mapTpd3 = vtkPolyDataMapper()
mapTpd3.SetInputConnection(outTpd3.GetOutputPort())
tpd3Actor = vtkActor()
tpd3Actor.SetMapper(mapTpd3)
tpd3Actor.GetProperty().SetColor(0,0,0)
appendF = vtkAppendPolyData()
appendF.AddInputConnection(tpd1.GetOutputPort())
appendF.AddInputConnection(tpd2.GetOutputPort())
appendF.AddInputConnection(tpd3.GetOutputPort())
probe = vtkProbeFilter()
probe.SetInputConnection(appendF.GetOutputPort())
probe.SetSourceData(output)
contour = vtkContourFilter()
contour.SetInputConnection(probe.GetOutputPort())
contour.GenerateValues(50,output.GetScalarRange())
contourMapper = vtkPolyDataMapper()
contourMapper.SetInputConnection(contour.GetOutputPort())
contourMapper.SetScalarRange(output.GetScalarRange())
planeActor = vtkActor()
planeActor.SetMapper(contourMapper)
outline = vtkStructuredGridOutlineFilter()
outline.SetInputData(output)
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(0,0,0)
ren1.AddActor(outlineActor)
ren1.AddActor(planeActor)
ren1.AddActor(tpd1Actor)
ren1.AddActor(tpd2Actor)
ren1.AddActor(tpd3Actor)
ren1.SetBackground(1,1,1)
renWin.SetSize(400,400)
cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)
iren.Initialize()
renWin.Render()
iren.Start()
# --- end of script --
