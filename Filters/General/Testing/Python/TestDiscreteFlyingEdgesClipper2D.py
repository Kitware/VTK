#!/usr/bin/env python
from vtkmodules.vtkFiltersGeneral import vtkDiscreteFlyingEdgesClipper2D
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import vtkImageExtractComponents
from vtkmodules.vtkImagingColor import vtkImageQuantizeRGBToIndex
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

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
ren1.SetBackground(1,1,1)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Read the data. Note this creates a 3-component scalar.
red = vtkPNGReader()
red.SetFileName(VTK_DATA_ROOT + "/Data/RedCircle.png")
red.Update()

# Next filter can only handle RGB *(&&*@
extract = vtkImageExtractComponents()
extract.SetInputConnection(red.GetOutputPort())
extract.SetComponents(0,1,2)

# Quantize the image into an index
quantize = vtkImageQuantizeRGBToIndex()
quantize.SetInputConnection(extract.GetOutputPort())
quantize.SetNumberOfColors(3)

# Create the pipeline
discrete = vtkDiscreteFlyingEdgesClipper2D()
discrete.SetInputConnection(quantize.GetOutputPort())
discrete.SetValue(0,1)

# Polygons are displayed
polyMapper = vtkPolyDataMapper()
polyMapper.SetInputConnection(discrete.GetOutputPort())

polyActor = vtkActor()
polyActor.SetMapper(polyMapper)

ren1.AddActor(polyActor)

renWin.Render()
iren.Start()
