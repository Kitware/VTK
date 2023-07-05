#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageThreshold
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

class TestThreshold(vtkmodules.test.Testing.vtkTest):

    def testThreshold(self):

        # This script is for testing the 3D threshold filter.

        # Image pipeline

        renWin = vtkRenderWindow()
        renWin.SetSize(192, 256)

        reader = vtkImageReader()
        reader.ReleaseDataFlagOff()
        reader.SetDataByteOrderToLittleEndian()
        reader.SetDataExtent(0, 63, 0, 63, 1, 93)
        reader.SetDataSpacing(3.2, 3.2, 1.5)
        reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
        reader.SetDataMask(0x7fff)

        outputtype = ["SignedChar", "UnsignedChar", "Long", "UnsignedLong", "Int", "UnsignedInt",
                        "Short", "UnsignedShort", "Double", "Float", "Double", "Float"]
        replacein = ["ReplaceInOn", "ReplaceInOff"]
        replaceout = ["ReplaceOutOn", "ReplaceOutOff"]
        thresholds = ["ThresholdByLower(800)", "ThresholdByUpper(1200)", "ThresholdBetween(800, 1200)"]

        thresh = list()
        map = list()
        act = list()
        ren = list()

        k = 0
        for rin in replacein:
            for rout in replaceout:
                for t in thresholds:

                    thresh.append(vtkImageThreshold())
                    thresh[k].SetInValue(2000)
                    thresh[k].SetOutValue(0)
                    eval('thresh[k].' + rin + '()')
                    eval('thresh[k].' + rout + '()')
                    thresh[k].SetInputConnection(reader.GetOutputPort())
                    eval('thresh[k].' + t)
                    eval('thresh[k].SetOutputScalarTypeTo' + outputtype[k] + '()')

                    map.append(vtkImageMapper())
                    map[k].SetInputConnection(thresh[k].GetOutputPort())
                    if k < 3:
                        map[k].SetColorWindow(255)
                        map[k].SetColorLevel(127.5)
                    else:
                        map[k].SetColorWindow(2000)
                        map[k].SetColorLevel(1000)

                    act.append(vtkActor2D())
                    act[k].SetMapper(map[k])

                    ren.append(vtkRenderer())
                    ren[k].AddActor2D(act[k])

                    renWin.AddRenderer(ren[k])

                    k += 1

        ren[0].SetViewport(0, 0, .33333, .25)
        ren[1].SetViewport(.33333, 0, .66667, .25)
        ren[2].SetViewport(.66667, 0, 1, .25)
        ren[3].SetViewport(0, .25, .33333, .5)
        ren[4].SetViewport(.33333, .25, .66667, .5)
        ren[5].SetViewport(.66667, .25, 1, .5)
        ren[6].SetViewport(0, .5, .33333, .75)
        ren[7].SetViewport(.33333, .5, .66667, .75)
        ren[8].SetViewport(.66667, .5, 1, .75)
        ren[9].SetViewport(0, .75, .33333, 1)
        ren[10].SetViewport(.33333, .75, .66667, 1)
        ren[11].SetViewport(.66667, .75, 1, 1)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "TestThreshold.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestThreshold, 'test')])
