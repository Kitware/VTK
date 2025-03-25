#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import (
    vtkImageMagnify,
    vtkImageShrink3D,
)
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

class TestAllShrinks(vtkmodules.test.Testing.vtkTest):

    def testAllShrinks(self):


        prefix = VTK_DATA_ROOT + "/Data/headsq/quarter"


        renWin = vtkRenderWindow()

        # Image pipeline
        reader = vtkImageReader()
        reader.SetDataExtent(0, 63, 0, 63, 1, 93)
        reader.SetFilePrefix(prefix)
        reader.SetDataByteOrderToLittleEndian()
        reader.SetDataMask(0x7fff)

        factor = 4
        magFactor = 8

        ops = ["Minimum", "Maximum", "Mean", "Median", "NoOp"]

        shrink = dict()
        mag = dict()
        mapper = dict()
        actor = dict()
        imager = dict()

        for operator in ops:
            shrink.update({operator:vtkImageShrink3D()})
            shrink[operator].SetMean(0)
            if operator != "NoOp":
             eval('shrink[operator].' + operator + 'On()')
            shrink[operator].SetShrinkFactors(factor, factor, factor)
            shrink[operator].SetInputConnection(reader.GetOutputPort())
            mag.update({operator:vtkImageMagnify()})
            mag[operator].SetMagnificationFactors(magFactor, magFactor, magFactor)
            mag[operator].InterpolateOff()
            mag[operator].SetInputConnection(shrink[operator].GetOutputPort())
            mapper.update({operator:vtkImageMapper()})
            mapper[operator].SetInputConnection(mag[operator].GetOutputPort())
            mapper[operator].SetColorWindow(2000)
            mapper[operator].SetColorLevel(1000)
            mapper[operator].SetZSlice(45)
            actor.update({operator:vtkActor2D()})
            actor[operator].SetMapper(mapper[operator])
            imager.update({operator:vtkRenderer()})
            imager[operator].AddViewProp(actor[operator])
            renWin.AddRenderer(imager[operator])

        shrink["Minimum"].Update
        shrink["Maximum"].Update
        shrink["Mean"].Update
        shrink["Median"].Update

        imager["Minimum"].SetViewport(0, 0, .5, .33)
        imager["Maximum"].SetViewport(0, .33, .5, .667)
        imager["Mean"].SetViewport(.5, 0, 1, .33)
        imager["Median"].SetViewport(.5, .33, 1, .667)
        imager["NoOp"].SetViewport(0, .667, 1, 1)

        renWin.SetSize(256, 384)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllShrinks.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestAllShrinks, 'test')])
