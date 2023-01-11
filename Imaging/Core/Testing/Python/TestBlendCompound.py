#!/usr/bin/env python

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkIOImage import (
    vtkBMPReader,
    vtkTIFFReader,
)
from vtkmodules.vtkImagingCore import (
    vtkImageAppendComponents,
    vtkImageBlend,
    vtkImageMapToColors,
    vtkImageShrink3D,
)
from vtkmodules.vtkImagingColor import vtkImageLuminance
from vtkmodules.vtkImagingSources import vtkImageCanvasSource2D
from vtkmodules.vtkRenderingCore import (
    vtkActor2D,
    vtkImageMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script calculates the luminance of an image

renWin = vtkRenderWindow()
renWin.SetSize(512, 256)

# Image pipeline

image1 = vtkTIFFReader()
image1.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")

# "beach.tif" image contains ORIENTATION tag which is
# ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
# reader parses this tag and sets the internal TIFF image
# orientation accordingly.  To overwrite this orientation with a vtk
# convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
# SetOrientationType method with parameter value of 4.
image1.SetOrientationType(4)

image2 = vtkBMPReader()
image2.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")

# shrink the images to a reasonable size

color = vtkImageShrink3D()
color.SetInputConnection(image1.GetOutputPort())
color.SetShrinkFactors(2, 2, 1)

backgroundColor = vtkImageShrink3D()
backgroundColor.SetInputConnection(image2.GetOutputPort())
backgroundColor.SetShrinkFactors(2, 2, 1)

# create a greyscale version

luminance = vtkImageLuminance()
luminance.SetInputConnection(color.GetOutputPort())

backgroundLuminance = vtkImageLuminance()
backgroundLuminance.SetInputConnection(backgroundColor.GetOutputPort())

# create an alpha mask

table = vtkLookupTable()
table.SetTableRange(220, 255)
table.SetValueRange(1, 0)
table.SetSaturationRange(0, 0)
table.Build()

alpha = vtkImageMapToColors()
alpha.SetInputConnection(luminance.GetOutputPort())
alpha.SetLookupTable(table)
alpha.SetOutputFormatToLuminance()

# make luminanceAlpha and colorAlpha versions

luminanceAlpha = vtkImageAppendComponents()
luminanceAlpha.AddInputConnection(luminance.GetOutputPort())
luminanceAlpha.AddInputConnection(alpha.GetOutputPort())

colorAlpha = vtkImageAppendComponents()
colorAlpha.AddInputConnection(color.GetOutputPort())
colorAlpha.AddInputConnection(alpha.GetOutputPort())

# create pseudo alpha values for background
bmask = vtkImageCanvasSource2D()
bmask.SetScalarTypeToUnsignedChar()
bmask.SetNumberOfScalarComponents(1)
bmask.SetExtent(0, 127, 0, 127, 0, 0)
bmask.SetDrawColor(0, 0, 0, 0)
bmask.FillBox(0, 127, 0, 127)
bmask.SetDrawColor(255, 0, 0, 0)
bmask.DrawCircle(64, 64, 40)
bmask.FillPixel(64, 64)

backgroundAlpha = vtkImageAppendComponents()
backgroundAlpha.AddInputConnection(backgroundColor.GetOutputPort())
backgroundAlpha.AddInputConnection(bmask.GetOutputPort())

foregrounds = ["luminance", "luminanceAlpha", "color", "colorAlpha"]
backgrounds = ["backgroundAlpha", "backgroundColor", "backgroundLuminance"]

deltaX = 1.0 / 4.0
deltaY = 1.0 / 3.0

blend = dict()
mapper = dict()
actor = dict()
imager = dict()

for row, bg in enumerate(backgrounds):
    for column, fg in enumerate(foregrounds):
        blend.update({bg:{fg:vtkImageBlend()}})
        blend[bg][fg].AddInputConnection(eval(bg + '.GetOutputPort()'))
        blend[bg][fg].SetBlendModeToCompound()
        if bg == "backgroundAlpha" or bg == "backgroundColor":
            blend[bg][fg].AddInputConnection(eval(fg + '.GetOutputPort()'))
            if bg == "backgroundAlpha":
                blend[bg][fg].SetCompoundAlpha(True)
                blend[bg][fg].SetOpacity(0, 0.5)
                blend[bg][fg].SetOpacity(1, 0.5)
            else:
                blend[bg][fg].SetOpacity(1, 0.8)
        elif fg == "luminance" or fg == "luminanceAlpha":
            blend[bg][fg].AddInputConnection(eval(fg + '.GetOutputPort()'))
            blend[bg][fg].SetOpacity(1, 0.8)

        mapper.update({bg:{fg:vtkImageMapper()}})
        mapper[bg][fg].SetInputConnection(blend[bg][fg].GetOutputPort())
        mapper[bg][fg].SetColorWindow(255)
        mapper[bg][fg].SetColorLevel(127.5)

        actor.update({bg:{fg:vtkActor2D()}})
        actor[bg][fg].SetMapper(mapper[bg][fg])

        imager.update({bg:{fg:vtkRenderer()}})
        imager[bg][fg].AddActor2D(actor[bg][fg])
        imager[bg][fg].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)
        imager[bg][fg].SetBackground(0.3, 0.3, 0.3)

        renWin.AddRenderer(imager[bg][fg])

        column += 1

# render and interact with data

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
renWin.Render()
iren.Start()
