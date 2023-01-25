#!/usr/bin/env python

from vtkmodules.vtkCommonDataModel import vtkPiecewiseFunction
from vtkmodules.vtkFiltersGeneral import vtkAxes
from vtkmodules.vtkFiltersModeling import vtkOutlineFilter
from vtkmodules.vtkIOImage import vtkSLCReader
from vtkmodules.vtkImagingCore import vtkImageMagnify
from vtkmodules.vtkImagingHybrid import vtkImageCursor3D
from vtkmodules.vtkInteractionImage import vtkImageViewer
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkColorTransferFunction,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderer,
    vtkVolume,
    vtkVolumeProperty,
)
from vtkmodules.vtkRenderingVolume import vtkFixedPointVolumeRayCastMapper
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingVolumeOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This little example shows how a cursor can be created in
# image viewers, and renderers.

# Global values

CURSOR_X = 20
CURSOR_Y = 20
CURSOR_Z = 20

IMAGE_MAG_X = 4
IMAGE_MAG_Y = 4
IMAGE_MAG_Z = 1

# Pipeline stuff

reader = vtkSLCReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/neghip.slc")

# Cursor stuff

magnify = vtkImageMagnify()
magnify.SetInputConnection(reader.GetOutputPort())
magnify.SetMagnificationFactors(IMAGE_MAG_X,IMAGE_MAG_Y,IMAGE_MAG_Z)

image_cursor = vtkImageCursor3D()
image_cursor.SetInputConnection(magnify.GetOutputPort())
image_cursor.SetCursorPosition(CURSOR_X*IMAGE_MAG_X,
                               CURSOR_Y*IMAGE_MAG_Y,
                               CURSOR_Z*IMAGE_MAG_Z)
image_cursor.SetCursorValue(255)
image_cursor.SetCursorRadius(50*IMAGE_MAG_X)

axes = vtkAxes()
axes.SymmetricOn()
axes.SetOrigin(CURSOR_X,CURSOR_Y,CURSOR_Z)
axes.SetScaleFactor(50.0)

axes_mapper = vtkPolyDataMapper()
axes_mapper.SetInputConnection(axes.GetOutputPort())

axesActor = vtkActor()
axesActor.SetMapper(axes_mapper)
axesActor.GetProperty().SetAmbient(0.5)

# Image viewer stuff

viewer = vtkImageViewer()
viewer.SetInputConnection(image_cursor.GetOutputPort())
viewer.SetZSlice(CURSOR_Z*IMAGE_MAG_Z)
viewer.SetColorWindow(256)
viewer.SetColorLevel(128)

# Create transfer functions for opacity and color

opacity_transfer_function = vtkPiecewiseFunction()
opacity_transfer_function.AddPoint(20, 0.0)
opacity_transfer_function.AddPoint(255, 0.2)

color_transfer_function = vtkColorTransferFunction()
color_transfer_function.AddRGBPoint(0, 0, 0, 0)
color_transfer_function.AddRGBPoint(64, 1, 0, 0)
color_transfer_function.AddRGBPoint(128, 0, 0, 1)
color_transfer_function.AddRGBPoint(192, 0, 1, 0)
color_transfer_function.AddRGBPoint(255, 0, .2, 0)

# Create properties, mappers, volume actors, and ray cast function

volume_property = vtkVolumeProperty()
volume_property.SetColor(color_transfer_function)
volume_property.SetScalarOpacity(opacity_transfer_function)

volume_mapper = vtkFixedPointVolumeRayCastMapper()
volume_mapper.SetInputConnection(reader.GetOutputPort())

volume = vtkVolume()
volume.SetMapper(volume_mapper)
volume.SetProperty(volume_property)

# Create outline

outline = vtkOutlineFilter()
outline.SetInputConnection(reader.GetOutputPort())

outline_mapper = vtkPolyDataMapper()
outline_mapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtkActor()
outlineActor.SetMapper(outline_mapper)
outlineActor.GetProperty().SetColor(1, 1, 1)

# Create the renderer

ren = vtkRenderer()
ren.AddActor(axesActor)
ren.AddVolume(volume)
ren.SetBackground(0.1, 0.2, 0.4)

renWin2 = vtkRenderWindow()
renWin2.AddRenderer(ren)
renWin2.SetSize(256, 256)

renWin2.Render()
