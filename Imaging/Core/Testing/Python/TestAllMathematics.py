#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkImagingMath import vtkImageMathematics
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

class TestAllMathematics(vtkmodules.test.Testing.vtkTest):

    def testAllMathematics(self):

        # append multiple displaced spheres into an RGB image.


        # Image pipeline

        renWin = vtkRenderWindow()

        sphere1 = vtkImageEllipsoidSource()
        sphere1.SetCenter(40, 20, 0)
        sphere1.SetRadius(30, 30, 0)
        sphere1.SetInValue(.75)
        sphere1.SetOutValue(.3)
        sphere1.SetOutputScalarTypeToFloat()
        sphere1.SetWholeExtent(0, 99, 0, 74, 0, 0)
        sphere1.Update()

        sphere2 = vtkImageEllipsoidSource()
        sphere2.SetCenter(60, 30, 0)
        sphere2.SetRadius(20, 20, 20)
        sphere2.SetInValue(.2)
        sphere2.SetOutValue(.5)
        sphere2.SetOutputScalarTypeToFloat()
        sphere2.SetWholeExtent(0, 99, 0, 74, 0, 0)
        sphere2.Update()

        mathematics = [ "Add", "Subtract", "Multiply", "Divide", "Invert", "Sin", "Cos",
                        "Exp", "Log", "AbsoluteValue", "Square", "SquareRoot", "Min",
                        "Max", "ATAN", "ATAN2", "MultiplyByK", "ReplaceCByK", "AddConstant"]

        mathematic = list()
        mapper = list()
        actor = list()
        imager = list()

        for idx, operator in enumerate(mathematics):
            mathematic.append(vtkImageMathematics())
            mathematic[idx].SetInput1Data(sphere1.GetOutput())
            mathematic[idx].SetInput2Data(sphere2.GetOutput())
            eval('mathematic[idx].SetOperationTo' + operator + '()')
            mathematic[idx].SetConstantK(.3)
            mathematic[idx].SetConstantC(.75)
            mapper.append(vtkImageMapper())
            mapper[idx].SetInputConnection(mathematic[idx].GetOutputPort())
            mapper[idx].SetColorWindow(2.0)
            mapper[idx].SetColorLevel(.75)
            actor.append(vtkActor2D())
            actor[idx].SetMapper(mapper[idx])
            imager.append(vtkRenderer())
            imager[idx].AddActor2D(actor[idx])
            renWin.AddRenderer(imager[idx])

        column = 1
        row = 1
        deltaX = 1.0 / 6.0
        deltaY = 1.0 / 4.0

        for idx, operator in enumerate(mathematics):
            imager[idx].SetViewport((column - 1) * deltaX, (row - 1) * deltaY, column * deltaX, row * deltaY)
            column += 1
            if column > 6:
                column = 1
                row += 1

        # Make the last operator finish the row
        vp = imager[len(mathematics) - 1].GetViewport()
        imager[len(mathematics) - 1].SetViewport(vp[0], vp[1], 1, 1)

        renWin.SetSize(600, 300)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestAllMathematics.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestAllMathematics, 'test')])
