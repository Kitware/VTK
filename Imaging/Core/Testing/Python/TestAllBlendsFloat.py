#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkIOImage import (
    vtkBMPReader,
    vtkTIFFReader,
)
from vtkmodules.vtkImagingCore import (
    vtkImageAppendComponents,
    vtkImageBlend,
    vtkImageShiftScale,
    vtkImageShrink3D,
    vtkImageThreshold,
)
from vtkmodules.vtkImagingColor import vtkImageLuminance
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

class TestAllBlendsFloat(vtkmodules.test.Testing.vtkTest):

    def testAllBlendsFloat(self):

        # This script blends images that consist of float data

        renWin = vtkRenderWindow()
        renWin.SetSize(512, 256)

        # Image pipeline

        inputImage = vtkTIFFReader()
        inputImage.SetFileName(VTK_DATA_ROOT + "/Data/beach.tif")

        # "beach.tif" image contains ORIENTATION tag which is
        # ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
        # reader parses this tag and sets the internal TIFF image
        # orientation accordingly.  To overwrite this orientation with a vtk
        # convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
        # SetOrientationType method with parameter value of 4.
        inputImage.SetOrientationType(4)

        inputImage2 = vtkBMPReader()
        inputImage2.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")

        # shrink the images to a reasonable size

        shrink1 = vtkImageShrink3D()
        shrink1.SetInputConnection(inputImage.GetOutputPort())
        shrink1.SetShrinkFactors(2, 2, 1)

        shrink2 = vtkImageShrink3D()
        shrink2.SetInputConnection(inputImage2.GetOutputPort())
        shrink2.SetShrinkFactors(2, 2, 1)

        color = vtkImageShiftScale()
        color.SetOutputScalarTypeToFloat()
        color.SetShift(0)
        color.SetScale(1.0 / 255)
        color.SetInputConnection(shrink1.GetOutputPort())

        backgroundColor = vtkImageShiftScale()
        backgroundColor.SetOutputScalarTypeToFloat()
        backgroundColor.SetShift(0)
        backgroundColor.SetScale(1.0 / 255)
        backgroundColor.SetInputConnection(shrink2.GetOutputPort())

        # create a greyscale version

        luminance = vtkImageLuminance()
        luminance.SetInputConnection(color.GetOutputPort())

        backgroundLuminance = vtkImageLuminance()
        backgroundLuminance.SetInputConnection(backgroundColor.GetOutputPort())

        # create an alpha mask

        alpha = vtkImageThreshold()
        alpha.SetInputConnection(luminance.GetOutputPort())
        alpha.ThresholdByLower(0.9)
        alpha.SetInValue(1.0)
        alpha.SetOutValue(0.0)

        # make luminanceAlpha and colorAlpha versions

        luminanceAlpha = vtkImageAppendComponents()
        luminanceAlpha.AddInputConnection(luminance.GetOutputPort())
        luminanceAlpha.AddInputConnection(alpha.GetOutputPort())

        colorAlpha = vtkImageAppendComponents()
        colorAlpha.AddInputConnection(color.GetOutputPort())
        colorAlpha.AddInputConnection(alpha.GetOutputPort())

        foregrounds = ["luminance", "luminanceAlpha", "color", "colorAlpha"]
        backgrounds = ["backgroundColor", "backgroundLuminance"]

        deltaX = 1.0 / 4.0
        deltaY = 1.0 / 2.0

        blend = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for row, bg in enumerate(backgrounds):
            for column, fg in enumerate(foregrounds):
                blend.update({bg:{fg:vtkImageBlend()}})
                blend[bg][fg].AddInputConnection(eval(bg + '.GetOutputPort()'))
                if bg == "backgroundColor" or fg == "luminance" or fg == "luminanceAlpha":
                    blend[bg][fg].AddInputConnection(eval(fg + '.GetOutputPort()'))
                    blend[bg][fg].SetOpacity(1, 0.8)

                mapper.update({bg:{fg:vtkImageMapper()}})
                mapper[bg][fg].SetInputConnection(blend[bg][fg].GetOutputPort())
                mapper[bg][fg].SetColorWindow(1.0)
                mapper[bg][fg].SetColorLevel(0.5)

                actor.update({bg:{fg:vtkActor2D()}})
                actor[bg][fg].SetMapper(mapper[bg][fg])

                imager.update({bg:{fg:vtkRenderer()}})
                imager[bg][fg].AddViewProp(actor[bg][fg])
                imager[bg][fg].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)

                renWin.AddRenderer(imager[bg][fg])

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllBlendsFloat.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestAllBlendsFloat, 'test')])
