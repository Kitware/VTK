#!/usr/bin/env python
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkCommonExecutionModel import (
    vtkCastToConcrete,
    vtkCompositeDataPipeline,
)
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkCutter,
    vtkPolyDataNormals,
    vtkTriangleFilter,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
from vtkmodules.vtkFiltersGeometry import vtkRectilinearGridGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOEnSight import vtkGenericEnSightReader
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
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

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

VTK_VARY_RADIUS_BY_VECTOR = 2

# create pipeline
#
# Make sure all algorithms use the composite data pipeline
cdp = vtkCompositeDataPipeline()

reader = vtkGenericEnSightReader()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/RectGrid_ascii.case")
reader.Update()

toRectilinearGrid = vtkCastToConcrete()
toRectilinearGrid.SetInputData(reader.GetOutput().GetBlock(0))
toRectilinearGrid.Update()

plane = vtkRectilinearGridGeometryFilter()
plane.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
plane.SetExtent(0, 100, 0, 100, 15, 15)

tri = vtkTriangleFilter()
tri.SetInputConnection(plane.GetOutputPort())

warper = vtkWarpVector()
warper.SetInputConnection(tri.GetOutputPort())
warper.SetScaleFactor(0.05)

planeMapper = vtkDataSetMapper()
planeMapper.SetInputConnection(warper.GetOutputPort())
planeMapper.SetScalarRange(0.197813, 0.710419)

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

cutPlane = vtkPlane()
cutPlane.SetOrigin(reader.GetOutput().GetBlock(0).GetCenter())
cutPlane.SetNormal(1, 0, 0)

planeCut = vtkCutter()
planeCut.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
planeCut.SetCutFunction(cutPlane)

cutMapper = vtkDataSetMapper()
cutMapper.SetInputConnection(planeCut.GetOutputPort())
cutMapper.SetScalarRange(
  reader.GetOutput().GetBlock(0).GetPointData().GetScalars().GetRange())

cutActor = vtkActor()
cutActor.SetMapper(cutMapper)

iso = vtkContourFilter()
iso.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
iso.SetValue(0, 0.7)

normals = vtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)

isoMapper = vtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('bisque'))
isoActor.GetProperty().SetRepresentationToWireframe()

streamer = vtkStreamTracer()
streamer.SetInputData(reader.GetOutput().GetBlock(0))
streamer.SetStartPosition(-1.2, -0.1, 1.3)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.05)
streamer.SetIntegrationDirectionToBoth()

streamTube = vtkTubeFilter()
streamTube.SetInputConnection(streamer.GetOutputPort())
streamTube.SetRadius(0.025)
streamTube.SetNumberOfSides(6)
streamTube.SetVaryRadius(VTK_VARY_RADIUS_BY_VECTOR)

mapStreamTube = vtkPolyDataMapper()
mapStreamTube.SetInputConnection(streamTube.GetOutputPort())
mapStreamTube.SetScalarRange(
  reader.GetOutput().GetBlock(0).GetPointData().GetScalars().GetRange())

streamTubeActor = vtkActor()
streamTubeActor.SetMapper(mapStreamTube)
streamTubeActor.GetProperty().BackfaceCullingOn()

outline = vtkOutlineFilter()
outline.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(outlineActor)
ren1.AddActor(planeActor)
ren1.AddActor(cutActor)
ren1.AddActor(isoActor)
ren1.AddActor(streamTubeActor)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(400, 400)

cam1 = ren1.GetActiveCamera()
cam1.SetClippingRange(3.76213, 10.712)
cam1.SetFocalPoint(-0.0842503, -0.136905, 0.610234)
cam1.SetPosition(2.53813, 2.2678, -5.22172)
cam1.SetViewUp(-0.241047, 0.930635, 0.275343)

reader.SetDefaultExecutivePrototype(None)

iren.Initialize()
# render the image
#
#iren.Start()
