#!/usr/bin/env python
import os
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# The current directory must be writeable.
#
try:
    channel = open("mni-surface-mesh-binary.obj", "wb")
    channel.close()

    ren1 = vtk.vtkRenderer()
    ren1.SetViewport(0, 0, 0.33, 1)
    ren2 = vtk.vtkRenderer()
    ren2.SetViewport(0.33, 0, 0.67, 1)
    ren3 = vtk.vtkRenderer()
    ren3.SetViewport(0.67, 0, 1, 1)
    renWin = vtk.vtkRenderWindow()
    renWin.SetSize(600, 200)
    renWin.AddRenderer(ren1)
    renWin.AddRenderer(ren2)
    renWin.AddRenderer(ren3)
    renWin.SetMultiSamples(0)
    property0 = vtk.vtkProperty()
    property0.SetDiffuseColor(0.95, 0.90, 0.70)
    filename = VTK_DATA_ROOT + "/Data/mni-surface-mesh.obj"
    asciiReader = vtk.vtkMNIObjectReader()
    property1 = asciiReader.GetProperty()
    if (asciiReader.CanReadFile(str(filename)) != 0):
        asciiReader.SetFileName(str(filename))

        # this is just to remove the normals, to increase coverage,
        # i.e. by forcing the writer to generate normals
        removeNormals = vtk.vtkClipClosedSurface()
        removeNormals.SetInputConnection(asciiReader.GetOutputPort())

        # this is to make triangle strips, also to increase coverage,
        # because it forces the writer to decompose the strips
        stripper = vtk.vtkStripper()
        stripper.SetInputConnection(removeNormals.GetOutputPort())

        # test binary writing and reading for polygons
        binaryWriter = vtk.vtkMNIObjectWriter()
        binaryWriter.SetInputConnection(stripper.GetOutputPort())
        binaryWriter.SetFileName("mni-surface-mesh-binary.obj")
        binaryWriter.SetProperty(property0)
        binaryWriter.SetFileTypeToBinary()
        binaryWriter.Write()

        binaryReader = vtk.vtkMNIObjectReader()
        binaryReader.SetFileName("mni-surface-mesh-binary.obj")

        property2 = binaryReader.GetProperty()

        # make a polyline object with color scalars
        scalars = vtk.vtkCurvatures()
        scalars.SetInputConnection(asciiReader.GetOutputPort())

        colors = vtk.vtkLookupTable()
        colors.SetRange(-14.5104, 29.0208)
        colors.SetAlphaRange(1.0, 1.0)
        colors.SetSaturationRange(1.0, 1.0)
        colors.SetValueRange(1.0, 1.0)
        colors.SetHueRange(0.0, 1.0)
        colors.Build()

        # this is just to test using the SetMapper option of vtkMNIObjectWriter
        mapper = vtk.vtkDataSetMapper()
        mapper.SetLookupTable(colors)
        mapper.UseLookupTableScalarRangeOn()
        edges = vtk.vtkExtractEdges()
        edges.SetInputConnection(scalars.GetOutputPort())
        # test ascii writing and reading for lines
        lineWriter = vtk.vtkMNIObjectWriter()
        lineWriter.SetMapper(mapper)
        # lineWriter SetLookupTable colors
        lineWriter.SetInputConnection(edges.GetOutputPort())
        lineWriter.SetFileName("mni-wire-mesh-ascii.obj")
        lineWriter.Write()

        lineReader = vtk.vtkMNIObjectReader()
        lineReader.SetFileName("mni-wire-mesh-ascii.obj")

        # display all the results
        mapper1 = vtk.vtkDataSetMapper()
        mapper1.SetInputConnection(asciiReader.GetOutputPort())

        mapper2 = vtk.vtkDataSetMapper()
        mapper2.SetInputConnection(binaryReader.GetOutputPort())

        mapper3 = vtk.vtkDataSetMapper()
        mapper3.SetInputConnection(lineReader.GetOutputPort())

        actor1 = vtk.vtkActor()
        actor1.SetMapper(mapper1)
        actor1.SetProperty(property1)

        actor2 = vtk.vtkActor()
        actor2.SetMapper(mapper2)
        actor2.SetProperty(property2)

        actor3 = vtk.vtkActor()
        actor3.SetMapper(mapper3)

        ren1.AddActor(actor1)
        ren2.AddActor(actor2)
        ren3.AddActor(actor3)

        iren = vtk.vtkRenderWindowInteractor()
        iren.SetRenderWindow(renWin)
        ren1.ResetCamera()
        ren1.GetActiveCamera().Dolly(1.2)
        ren1.ResetCameraClippingRange()
        ren2.ResetCamera()
        ren2.GetActiveCamera().Dolly(1.2)
        ren2.ResetCameraClippingRange()
        ren3.ResetCamera()
        ren3.GetActiveCamera().Dolly(1.2)
        ren3.ResetCameraClippingRange()
        iren.Render()

        # cleanup
        #
        try:
            os.remove("mni-surface-mesh-binary.obj")
        except OSError:
            pass
        try:
            os.remove("mni-wire-mesh-ascii.obj")
        except OSError:
            pass

        # render the image
        #
        iren.Initialize()
#        iren.Start()

except IOError:
    print  "Unable to test the writer/reader."
