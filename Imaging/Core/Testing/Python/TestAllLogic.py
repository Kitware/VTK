#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkImagingMath import vtkImageLogic
from vtkmodules.vtkImagingSources import vtkImageEllipsoidSource
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

class TestAllLogic(vtkmodules.test.Testing.vtkTest):

    def testAllLogic(self):

        # append multiple displaced spheres into an RGB image.


        # Image pipeline

        renWin = vtkRenderWindow()

        logics = ["And", "Or", "Xor", "Nand", "Nor", "Not"]
        types = ["Float", "Double", "UnsignedInt", "UnsignedLong", "UnsignedShort", "UnsignedChar"]

        sphere1 = list()
        sphere2 = list()
        logic = list()
        mapper = list()
        actor = list()
        imager = list()

        for idx, operator in enumerate(logics):
            ScalarType = types[idx]

            sphere1.append(vtkImageEllipsoidSource())
            sphere1[idx].SetCenter(95, 100, 0)
            sphere1[idx].SetRadius(70, 70, 70)
            eval('sphere1[idx].SetOutputScalarTypeTo' + ScalarType + '()')
            sphere1[idx].Update()

            sphere2.append(vtkImageEllipsoidSource())
            sphere2[idx].SetCenter(161, 100, 0)
            sphere2[idx].SetRadius(70, 70, 70)
            eval('sphere2[idx].SetOutputScalarTypeTo' + ScalarType + '()')
            sphere2[idx].Update()

            logic.append(vtkImageLogic())
            logic[idx].SetInput1Data(sphere1[idx].GetOutput())
            if operator != "Not":
                logic[idx].SetInput2Data(sphere2[idx].GetOutput())

            logic[idx].SetOutputTrueValue(150)
            eval('logic[idx].SetOperationTo' + operator + '()')

            mapper.append(vtkImageMapper())
            mapper[idx].SetInputConnection(logic[idx].GetOutputPort())
            mapper[idx].SetColorWindow(255)
            mapper[idx].SetColorLevel(127.5)

            actor.append(vtkActor2D())
            actor[idx].SetMapper(mapper[idx])

            imager.append(vtkRenderer())
            imager[idx].AddActor2D(actor[idx])

            renWin.AddRenderer(imager[idx])



        imager[0].SetViewport(0, .5, .33, 1)
        imager[1].SetViewport(.33, .5, .66, 1)
        imager[2].SetViewport(.66, .5, 1, 1)
        imager[3].SetViewport(0, 0, .33, .5)
        imager[4].SetViewport(.33, 0, .66, .5)
        imager[5].SetViewport(.66, 0, 1, .5)

        renWin.SetSize(768, 512)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllLogic.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestAllLogic, 'test')])
