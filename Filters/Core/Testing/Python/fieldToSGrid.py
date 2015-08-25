#!/usr/bin/env python
import os
import vtk
from vtk.test import Testing
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

# Demonstrate the generation of a structured grid from field data. The output
# should be similar to combIso.tcl.
#
# NOTE: This test only works if the current directory is writable
#
try:
    channel = open("combsg.vtk", "wb")
    channel.close()
    channel = open("SGridField.vtk", "wb")
    channel.close()

    # Create a reader and write out the field
    comb = vtk.vtkMultiBlockPLOT3DReader()
    comb.SetXYZFileName(VTK_DATA_ROOT + "/Data/combxyz.bin")
    comb.SetQFileName(VTK_DATA_ROOT + "/Data/combq.bin")
    comb.SetScalarFunctionNumber(100)
    comb.Update()

    output = comb.GetOutput().GetBlock(0)

    wsg = vtk.vtkStructuredGridWriter()
    wsg.SetInputData(output)
    wsg.SetFileTypeToBinary()
    wsg.SetFileName("combsg.vtk")
    wsg.Write()

    pl3d = vtk.vtkStructuredGridReader()
    pl3d.SetFileName("combsg.vtk")

    ds2do = vtk.vtkDataSetToDataObjectFilter()
    ds2do.SetInputConnection(pl3d.GetOutputPort())

    writer = vtk.vtkDataObjectWriter()
    writer.SetInputConnection(ds2do.GetOutputPort())
    writer.SetFileName("SGridField.vtk")
    writer.Write()

    # read the field
    dor = vtk.vtkDataObjectReader()
    dor.SetFileName("SGridField.vtk")

    do2ds = vtk.vtkDataObjectToDataSetFilter()
    do2ds.SetInputConnection(dor.GetOutputPort())
    do2ds.SetDataSetTypeToStructuredGrid()
    do2ds.SetDimensionsComponent("Dimensions", 0)
    do2ds.SetPointComponent(0, "Points", 0)
    do2ds.SetPointComponent(1, "Points", 1)
    do2ds.SetPointComponent(2, "Points", 2)
    do2ds.Update()

    fd2ad = vtk.vtkFieldDataToAttributeDataFilter()
    fd2ad.SetInputData(do2ds.GetStructuredGridOutput())
    fd2ad.SetInputFieldToDataObjectField()
    fd2ad.SetOutputAttributeDataToPointData()
    fd2ad.SetVectorComponent(0, "Momentum", 0)
    fd2ad.SetVectorComponent(1, "Momentum", 1)
    fd2ad.SetVectorComponent(2, "Momentum", 2)
    fd2ad.SetScalarComponent(0, "Density", 0)
    fd2ad.Update()

    # create pipeline
    #
    iso = vtk.vtkContourFilter()
    iso.SetInputConnection(fd2ad.GetOutputPort())
    iso.SetValue(0, .38)

    normals = vtk.vtkPolyDataNormals()
    normals.SetInputConnection(iso.GetOutputPort())
    normals.SetFeatureAngle(45)

    isoMapper = vtk.vtkPolyDataMapper()
    isoMapper.SetInputConnection(normals.GetOutputPort())
    isoMapper.ScalarVisibilityOff()
    isoActor = vtk.vtkActor()
    isoActor.SetMapper(isoMapper)
    isoActor.GetProperty().SetColor(GetRGBColor('bisque'))

    outline = vtk.vtkStructuredGridOutlineFilter()
    outline.SetInputData(fd2ad.GetStructuredGridOutput())

    outlineMapper = vtk.vtkPolyDataMapper()
    outlineMapper.SetInputConnection(outline.GetOutputPort())

    outlineActor = vtk.vtkActor()
    outlineActor.SetMapper(outlineMapper)

    # Create the RenderWindow, Renderer and both Actors
    #
    ren1 = vtk.vtkRenderer()
    renWin = vtk.vtkRenderWindow()
    renWin.AddRenderer(ren1)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)

    # Add the actors to the renderer, set the background and size
    #
    ren1.AddActor(outlineActor)
    ren1.AddActor(isoActor)
    ren1.SetBackground(1, 1, 1)

    renWin.SetSize(250, 250)

    ren1.SetBackground(0.1, 0.2, 0.4)

    cam1 = ren1.GetActiveCamera()
    cam1.SetClippingRange(3.95297, 50)
    cam1.SetFocalPoint(9.71821, 0.458166, 29.3999)
    cam1.SetPosition(2.7439, -37.3196, 38.7167)
    cam1.SetViewUp(-0.16123, 0.264271, 0.950876)

    # render the image
    #
    renWin.Render()

    # cleanup
    #
    try:
        os.remove("combsg.vtk")
    except OSError:
        pass
    try:
        os.remove("SGridField.vtk")
    except OSError:
        pass

#    iren.Start()

except IOError:
    print("Couldn't open combsg.vtk or SGridField.vtk for writing.")
