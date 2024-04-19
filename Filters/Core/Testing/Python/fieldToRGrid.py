#!/usr/bin/env python
import os
from vtkmodules.vtkCommonColor import vtkNamedColors
from vtkmodules.vtkCommonDataModel import vtkPlane
from vtkmodules.vtkFiltersCore import (
    vtkContourFilter,
    vtkCutter,
    vtkDataObjectToDataSetFilter,
    vtkDataSetToDataObjectFilter,
    vtkFieldDataToAttributeDataFilter,
    vtkPolyDataNormals,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersFlowPaths import vtkStreamTracer
from vtkmodules.vtkFiltersGeneral import vtkWarpVector
from vtkmodules.vtkFiltersGeometry import vtkRectilinearGridGeometryFilter
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOLegacy import (
    vtkDataObjectReader,
    vtkDataObjectWriter,
    vtkDataSetReader,
)
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

# Generate a rectilinear grid from a field.
#

# Create a reader and write out the field
reader = vtkDataSetReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/RectGrid2.vtk")

ds2do = vtkDataSetToDataObjectFilter()
ds2do.SetInputConnection(reader.GetOutputPort())

# NOTE: This test only works if the current directory is writable
#
try:
    channel = open("RGridField.vtk", "wb")
    channel.close()

    writer = vtkDataObjectWriter()
    writer.SetInputConnection(ds2do.GetOutputPort())
    writer.SetFileName("RGridField.vtk")
    writer.Write()

    # Read the field
    #
    dor = vtkDataObjectReader()
    dor.SetFileName("RGridField.vtk")

    do2ds = vtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToRectilinearGrid()
    do2ds.SetDimensionsComponent("Dimensions", 0)
    do2ds.SetPointComponent(0, "XCoordinates", 0)
    do2ds.SetPointComponent(1, "YCoordinates", 0)
    do2ds.SetPointComponent(2, "ZCoordinates", 0)
    do2ds.Update()

    fd2ad = vtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputData(do2ds.GetRectilinearGridOutput())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetVectorComponent(0, "vectors", 0)
    fd2ad.SetVectorComponent(1, "vectors", 1)
    fd2ad.SetVectorComponent(2, "vectors", 2)
    fd2ad.SetScalarComponent(0, "scalars", 0)
    fd2ad.Update()

    # create pipeline
    #
    plane = vtkRectilinearGridGeometryFilter()
    plane.SetInputData(fd2ad.GetRectilinearGridOutput())
    plane.SetExtent(0, 100, 0, 100, 15, 15)

    warper = vtkWarpVector()
    warper.SetInputConnection(plane.GetOutputPort())
    warper.SetScaleFactor(0.05)

    planeMapper = vtkDataSetMapper()
    planeMapper.SetInputConnection(warper.GetOutputPort())
    planeMapper.SetScalarRange(0.197813, 0.710419)

    planeActor = vtkActor()
    planeActor.SetMapper(planeMapper)

    cutPlane = vtkPlane()
    cutPlane.SetOrigin(fd2ad.GetOutput().GetCenter())
    cutPlane.SetNormal(1, 0, 0)

    planeCut = vtkCutter()
    planeCut.SetInputData(fd2ad.GetRectilinearGridOutput())
    planeCut.SetCutFunction(cutPlane)

    cutMapper = vtkDataSetMapper()
    cutMapper.SetInputConnection(planeCut.GetOutputPort())
    cutMapper.SetScalarRange(fd2ad.GetOutput().GetPointData().GetScalars().GetRange())

    cutActor = vtkActor()
    cutActor.SetMapper(cutMapper)

    iso = vtkContourFilter()
    iso.SetInputData(fd2ad.GetRectilinearGridOutput())
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
    streamer.SetInputConnection(fd2ad.GetOutputPort())
    streamer.SetStartPosition(-1.2, -0.1, 1.3)
    streamer.SetMaximumPropagation(500)
    streamer.SetInitialIntegrationStep(0.05)
    streamer.SetIntegrationDirectionToBoth()

    streamTube = vtkTubeFilter()
    streamTube.SetInputConnection(streamer.GetOutputPort())
    streamTube.SetRadius(0.025)
    streamTube.SetNumberOfSides(6)
    streamTube.SetVaryRadiusToVaryRadiusByVector()

    mapStreamTube = vtkPolyDataMapper()
    mapStreamTube.SetInputConnection(streamTube.GetOutputPort())
    mapStreamTube.SetScalarRange(fd2ad.GetOutput().GetPointData().GetScalars().GetRange())

    streamTubeActor = vtkActor()
    streamTubeActor.SetMapper(mapStreamTube)
    streamTubeActor.GetProperty().BackfaceCullingOn()

    outline = vtkOutlineFilter()
    outline.SetInputData(fd2ad.GetRectilinearGridOutput())

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

    renWin.SetSize(300, 300)

    ren1.GetActiveCamera().SetPosition(0.0390893, 0.184813, -3.94026)
    ren1.GetActiveCamera().SetFocalPoint(-0.00578326, 0, 0.701967)
    ren1.GetActiveCamera().SetViewAngle(30)
    ren1.GetActiveCamera().SetViewUp(0.00850257, 0.999169, 0.0398605)
    ren1.GetActiveCamera().SetClippingRange(3.08127, 6.62716)

    iren.Initialize()

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("RGridField.vtk")
    except OSError:
        pass


#    iren.Start()

except IOError:
    print("Couldn't open RGridField.vtk for writing.")
