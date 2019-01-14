#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def GetRGBColor(colorName):
    '''
        Return the red, green and blue components for a
        color as doubles.
    '''
    rgb = [0.0, 0.0, 0.0]  # black
    vtk.vtkNamedColors().GetColorRGB(colorName, rgb)
    return rgb

VTK_VARY_RADIUS_BY_VECTOR = 2

# create pipeline
#
# Make sure all algorithms use the composite data pipeline
cdp = vtk.vtkCompositeDataPipeline()

reader = vtk.vtkGenericEnSightReader()
reader.SetDefaultExecutivePrototype(cdp)
reader.SetCaseFileName(VTK_DATA_ROOT + "/Data/EnSight/RectGrid_ascii.case")
reader.Update()

toRectilinearGrid = vtk.vtkCastToConcrete()
toRectilinearGrid.SetInputData(reader.GetOutput().GetBlock(0))
toRectilinearGrid.Update()

plane = vtk.vtkRectilinearGridGeometryFilter()
plane.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
plane.SetExtent(0, 100, 0, 100, 15, 15)

tri = vtk.vtkTriangleFilter()
tri.SetInputConnection(plane.GetOutputPort())

warper = vtk.vtkWarpVector()
warper.SetInputConnection(tri.GetOutputPort())
warper.SetScaleFactor(0.05)

planeMapper = vtk.vtkDataSetMapper()
planeMapper.SetInputConnection(warper.GetOutputPort())
planeMapper.SetScalarRange(0.197813, 0.710419)

planeActor = vtk.vtkActor()
planeActor.SetMapper(planeMapper)

cutPlane = vtk.vtkPlane()
cutPlane.SetOrigin(reader.GetOutput().GetBlock(0).GetCenter())
cutPlane.SetNormal(1, 0, 0)

planeCut = vtk.vtkCutter()
planeCut.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
planeCut.SetCutFunction(cutPlane)

cutMapper = vtk.vtkDataSetMapper()
cutMapper.SetInputConnection(planeCut.GetOutputPort())
cutMapper.SetScalarRange(
  reader.GetOutput().GetBlock(0).GetPointData().GetScalars().GetRange())

cutActor = vtk.vtkActor()
cutActor.SetMapper(cutMapper)

iso = vtk.vtkContourFilter()
iso.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())
iso.SetValue(0, 0.7)

normals = vtk.vtkPolyDataNormals()
normals.SetInputConnection(iso.GetOutputPort())
normals.SetFeatureAngle(45)

isoMapper = vtk.vtkPolyDataMapper()
isoMapper.SetInputConnection(normals.GetOutputPort())
isoMapper.ScalarVisibilityOff()

isoActor = vtk.vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(GetRGBColor('bisque'))
isoActor.GetProperty().SetRepresentationToWireframe()

streamer = vtk.vtkStreamTracer()
streamer.SetInputData(reader.GetOutput().GetBlock(0))
streamer.SetStartPosition(-1.2, -0.1, 1.3)
streamer.SetMaximumPropagation(500)
streamer.SetInitialIntegrationStep(0.05)
streamer.SetIntegrationDirectionToBoth()

streamTube = vtk.vtkTubeFilter()
streamTube.SetInputConnection(streamer.GetOutputPort())
streamTube.SetRadius(0.025)
streamTube.SetNumberOfSides(6)
streamTube.SetVaryRadius(VTK_VARY_RADIUS_BY_VECTOR)

mapStreamTube = vtk.vtkPolyDataMapper()
mapStreamTube.SetInputConnection(streamTube.GetOutputPort())
mapStreamTube.SetScalarRange(
  reader.GetOutput().GetBlock(0).GetPointData().GetScalars().GetRange())

streamTubeActor = vtk.vtkActor()
streamTubeActor.SetMapper(mapStreamTube)
streamTubeActor.GetProperty().BackfaceCullingOn()

outline = vtk.vtkOutlineFilter()
outline.SetInputData(toRectilinearGrid.GetRectilinearGridOutput())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(GetRGBColor('black'))

# Graphics stuff
# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
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
