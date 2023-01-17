#!/usr/bin/env python
import os
from vtkmodules.vtkCommonCore import (
    vtkDoubleArray,
    vtkStringArray,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import vtkGlyph3D
from vtkmodules.vtkFiltersGeneral import vtkTransformFilter
from vtkmodules.vtkFiltersSources import (
    vtkPointSource,
    vtkSphereSource,
)
from vtkmodules.vtkIOMINC import (
    vtkMNITagPointReader,
    vtkMNITagPointWriter,
)
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkActor2D,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkTextProperty,
)
from vtkmodules.vtkRenderingLabel import (
    vtkLabelPlacementMapper,
    vtkPointSetToLabelHierarchy,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
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
    sphere1 = vtkPointSource()
    sphere1.SetNumberOfPoints(13)

    xform = vtkTransform()
    xform.RotateWXYZ(20, 1, 0, 0)

    xformFilter = vtkTransformFilter()
    xformFilter.SetTransform(xform)
    xformFilter.SetInputConnection(sphere1.GetOutputPort())

    labels = vtkStringArray()
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

    weights = vtkDoubleArray()
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

    writer = vtkMNITagPointWriter()
    writer.SetFileName(fname)
    writer.SetInputConnection(sphere1.GetOutputPort())
    writer.SetInputConnection(1, xformFilter.GetOutputPort())
    writer.SetLabelText(labels)
    writer.SetWeights(weights)
    writer.SetComments("Volume 1: sphere points\nVolume 2: transformed points")
    writer.Write()

    reader = vtkMNITagPointReader()
    reader.CanReadFile(fname)
    reader.SetFileName(fname)

    textProp = vtkTextProperty()
    textProp.SetFontSize(12)
    textProp.SetColor(1.0, 1.0, 0.5)

    labelHier = vtkPointSetToLabelHierarchy()
    labelHier.SetInputConnection(reader.GetOutputPort())
    labelHier.SetTextProperty(textProp)
    labelHier.SetLabelArrayName("LabelText")
    labelHier.SetMaximumDepth(15)
    labelHier.SetTargetLabelCount(12)

    labelMapper = vtkLabelPlacementMapper()
    labelMapper.SetInputConnection(labelHier.GetOutputPort())
    labelMapper.UseDepthBufferOff()
    labelMapper.SetShapeToRect()
    labelMapper.SetStyleToOutline()

    labelActor = vtkActor2D()
    labelActor.SetMapper(labelMapper)

    glyphSource = vtkSphereSource()
    glyphSource.SetRadius(0.01)

    glyph = vtkGlyph3D()
    glyph.SetSourceConnection(glyphSource.GetOutputPort())
    glyph.SetInputConnection(reader.GetOutputPort())

    mapper = vtkDataSetMapper()
    mapper.SetInputConnection(glyph.GetOutputPort())

    actor = vtkActor()
    actor.SetMapper(mapper)

    # Create rendering stuff
    ren1 = vtkRenderer()
    renWin = vtkRenderWindow()
    renWin.SetMultiSamples(0)
    renWin.AddRenderer(ren1)
    iren = vtkRenderWindowInteractor()
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
