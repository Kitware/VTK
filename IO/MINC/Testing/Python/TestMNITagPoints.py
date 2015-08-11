#!/usr/bin/env python
import os
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test label reading from an MNI tag file
#

# The current directory must be writeable.
#
try:
    fname = "mni-tagtest.tag"
    channel = open(fname, "wb")
    channel.close()

    # create some random points in a sphere
    #
    sphere1 = vtk.vtkPointSource()
    sphere1.SetNumberOfPoints(13)

    xform = vtk.vtkTransform()
    xform.RotateWXYZ(20, 1, 0, 0)

    xformFilter = vtk.vtkTransformFilter()
    xformFilter.SetTransform(xform)
    xformFilter.SetInputConnection(sphere1.GetOutputPort())

    labels = vtk.vtkStringArray()
    labels.InsertNextValue("0")
    labels.InsertNextValue("1")
    labels.InsertNextValue("2")
    labels.InsertNextValue("3")
    labels.InsertNextValue("Halifax")
    labels.InsertNextValue("Toronto")
    labels.InsertNextValue("Vancouver")
    labels.InsertNextValue("Larry")
    labels.InsertNextValue("Bob")
    labels.InsertNextValue("Jackie")
    labels.InsertNextValue("10")
    labels.InsertNextValue("11")
    labels.InsertNextValue("12")

    weights = vtk.vtkDoubleArray()
    weights.InsertNextValue(1.0)
    weights.InsertNextValue(1.1)
    weights.InsertNextValue(1.2)
    weights.InsertNextValue(1.3)
    weights.InsertNextValue(1.4)
    weights.InsertNextValue(1.5)
    weights.InsertNextValue(1.6)
    weights.InsertNextValue(1.7)
    weights.InsertNextValue(1.8)
    weights.InsertNextValue(1.9)
    weights.InsertNextValue(0.9)
    weights.InsertNextValue(0.8)
    weights.InsertNextValue(0.7)

    writer = vtk.vtkMNITagPointWriter()
    writer.SetFileName(fname)
    writer.SetInputConnection(sphere1.GetOutputPort())
    writer.SetInputConnection(1, xformFilter.GetOutputPort())
    writer.SetLabelText(labels)
    writer.SetWeights(weights)
    writer.SetComments("Volume 1: sphere points\nVolume 2: transformed points")
    writer.Write()

    reader = vtk.vtkMNITagPointReader()
    reader.CanReadFile(fname)
    reader.SetFileName(fname)

    textProp = vtk.vtkTextProperty()
    textProp.SetFontSize(12)
    textProp.SetColor(1.0, 1.0, 0.5)

    labelHier = vtk.vtkPointSetToLabelHierarchy()
    labelHier.SetInputConnection(reader.GetOutputPort())
    labelHier.SetTextProperty(textProp)
    labelHier.SetLabelArrayName("LabelText")
    labelHier.SetMaximumDepth(15)
    labelHier.SetTargetLabelCount(12)

    labelMapper = vtk.vtkLabelPlacementMapper()
    labelMapper.SetInputConnection(labelHier.GetOutputPort())
    labelMapper.UseDepthBufferOff()
    labelMapper.SetShapeToRect()
    labelMapper.SetStyleToOutline()

    labelActor = vtk.vtkActor2D()
    labelActor.SetMapper(labelMapper)

    glyphSource = vtk.vtkSphereSource()
    glyphSource.SetRadius(0.01)

    glyph = vtk.vtkGlyph3D()
    glyph.SetSourceConnection(glyphSource.GetOutputPort())
    glyph.SetInputConnection(reader.GetOutputPort())

    mapper = vtk.vtkDataSetMapper()
    mapper.SetInputConnection(glyph.GetOutputPort())

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)

    # Create rendering stuff
    ren1 = vtk.vtkRenderer()
    renWin = vtk.vtkRenderWindow()
    renWin.SetMultiSamples(0)
    renWin.AddRenderer(ren1)
    iren = vtk.vtkRenderWindowInteractor()
    iren.SetRenderWindow(renWin)
    # Add the actors to the renderer, set the background and size
    #
    ren1.AddViewProp(actor)
    ren1.AddViewProp(labelActor)
    ren1.SetBackground(0, 0, 0)

    renWin.SetSize(300, 300)

    renWin.Render()
    try:
        os.remove(fname)
    except OSError:
        pass

    # render the image
    #
#    iren.Start()

except IOError:
    print("Unable to test the writer/reader.")
