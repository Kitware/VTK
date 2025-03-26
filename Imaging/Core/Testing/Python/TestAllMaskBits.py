#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkIOImage import vtkTIFFReader
from vtkmodules.vtkImagingCore import vtkImageShrink3D
from vtkmodules.vtkImagingMath import vtkImageMaskBits
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

class TestAllMaskBits(vtkmodules.test.Testing.vtkTest):

    def testAllMaskBits(self):

        # This script calculates the luminance of an image

        renWin = vtkRenderWindow()


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

        shrink = vtkImageShrink3D()
        shrink.SetInputConnection(image1.GetOutputPort())
        shrink.SetShrinkFactors(2, 2, 1)

        operators = ["ByPass", "And", "Nand", "Xor", "Or", "Nor"]

        operator = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for idx, op in enumerate(operators):
            if op != "ByPass":
                operator.update({idx: vtkImageMaskBits()})
                operator[idx].SetInputConnection(shrink.GetOutputPort())
                eval('operator[' + str(idx) + '].SetOperationTo' + op + '()')
                operator[idx].SetMasks(255, 255, 0)

            mapper.update({idx: vtkImageMapper()})
            if op != "ByPass":
                mapper[idx].SetInputConnection(operator[idx].GetOutputPort())
            else:
                mapper[idx].SetInputConnection(shrink.GetOutputPort())
            mapper[idx].SetColorWindow(255)
            mapper[idx].SetColorLevel(127.5)

            actor.update({idx: vtkActor2D()})
            actor[idx].SetMapper(mapper[idx])

            imager.update({idx: vtkRenderer()})
            imager[idx].AddActor2D(actor[idx])

            renWin.AddRenderer(imager[idx])


        column = 0
        row = 0
        deltaX = 1.0 / 3.0
        deltaY = 1.0 / 2.0

        for idx in range(len(operators)):
            imager[idx].SetViewport(column * deltaX, row * deltaY, (column + 1) * deltaX, (row + 1) * deltaY)
            column += 1
            if column > 2:
                column = 0
                row += 1

        renWin.SetSize(384, 256)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllMaskBits.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestAllMaskBits, 'test')])
