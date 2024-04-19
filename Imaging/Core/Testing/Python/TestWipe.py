#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkImagingHybrid import vtkImageRectilinearWipe
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

class TestWipe(vtkmodules.test.Testing.vtkTest):

    def testWipe(self):

        # Image pipeline

        renWin = vtkRenderWindow()

        image1 = vtkImageCanvasSource2D()
        image1.SetNumberOfScalarComponents(3)
        image1.SetScalarTypeToUnsignedChar()
        image1.SetExtent(0, 79, 0, 79, 0, 0)
        image1.SetDrawColor(255, 255, 0)
        image1.FillBox(0, 79, 0, 79)
        image1.Update()

        image2 = vtkImageCanvasSource2D()
        image2.SetNumberOfScalarComponents(3)
        image2.SetScalarTypeToUnsignedChar()
        image2.SetExtent(0, 79, 0, 79, 0, 0)
        image2.SetDrawColor(0, 255, 255)
        image2.FillBox(0, 79, 0, 79)
        image2.Update()

        mapper = vtkImageMapper()
        mapper.SetInputConnection(image1.GetOutputPort())
        mapper.SetColorWindow(255)
        mapper.SetColorLevel(127.5)
        actor = vtkActor2D()
        actor.SetMapper(mapper)
        imager = vtkRenderer()
        imager.AddActor2D(actor)

        renWin.AddRenderer(imager)

        wipes = ["Quad", "Horizontal", "Vertical", "LowerLeft", "LowerRight", "UpperLeft", "UpperRight"]

        wiper = dict()
        mapper = dict()
        actor = dict()
        imagers = dict()

        for wipe in wipes:
            wiper.update({wipe:vtkImageRectilinearWipe()})
            wiper[wipe].SetInput1Data(image1.GetOutput())
            wiper[wipe].SetInput2Data(image2.GetOutput())
            wiper[wipe].SetPosition(20, 20)
            eval('wiper[wipe].SetWipeTo' + wipe + '()')

            mapper.update({wipe:vtkImageMapper()})
            mapper[wipe].SetInputConnection(wiper[wipe].GetOutputPort())
            mapper[wipe].SetColorWindow(255)
            mapper[wipe].SetColorLevel(127.5)

            actor.update({wipe:vtkActor2D()})
            actor[wipe].SetMapper(mapper[wipe])

            imagers.update({wipe:vtkRenderer()})
            imagers[wipe].AddActor2D(actor[wipe])

            renWin.AddRenderer(imagers[wipe])

        imagers["Quad"].SetViewport(0, .5, .25, 1)
        imagers["Horizontal"].SetViewport(.25, .5, .5, 1)
        imagers["Vertical"].SetViewport(.5, .5, .75, 1)
        imagers["LowerLeft"].SetViewport(.75, .5, 1, 1)
        imagers["LowerRight"].SetViewport(0, 0, .25, .5)
        imagers["UpperLeft"].SetViewport(.25, 0, .5, .5)
        imagers["UpperRight"].SetViewport(.5, 0, .75, .5)
        imager.SetViewport(.75, 0, 1, .5)

        renWin.SetSize(400, 200)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestWipe.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestWipe, 'test')])
