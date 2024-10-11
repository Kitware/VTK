#!/usr/bin/env python
# -*- coding: utf-8 -*-



import sys
from vtkmodules.vtkCommonDataModel import vtkPiecewiseFunction
from vtkmodules.vtkIOXML import vtkXMLImageDataReader
from vtkmodules.vtkRenderingCore import (
    vtkColorTransferFunction,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkVolume,
)
from vtkmodules.vtkRenderingVolume import vtkGPUVolumeRayCastMapper
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingVolumeOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

class TestGPURayCastIndependentComponentMIP(vtkmodules.test.Testing.vtkTest):

    def test(self):
        dataRoot = vtkGetDataRoot()
        reader =  vtkXMLImageDataReader()
        reader.SetFileName(dataRoot + "/Data/vase_4comp.vti")

        volume = vtkVolume()
        #mapper = vtkFixedPointVolumeRayCastMapper()
        mapper = vtkGPUVolumeRayCastMapper()
        mapper.SetBlendModeToMaximumIntensity();
        mapper.SetSampleDistance(0.1)
        mapper.SetAutoAdjustSampleDistances(0)
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        iRen = vtkRenderWindowInteractor()

        # Set connections
        mapper.SetInputConnection(reader.GetOutputPort());
        volume.SetMapper(mapper)
        ren.AddViewProp(volume)
        renWin.AddRenderer(ren)
        iRen.SetRenderWindow(renWin)

        # Define opacity transfer function and color functions
        opacityFunc1 = vtkPiecewiseFunction()
        opacityFunc1.AddPoint(0.0, 0.0);
        opacityFunc1.AddPoint(60.0, 0.1);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc2 = vtkPiecewiseFunction()
        opacityFunc2.AddPoint(0.0, 0.0);
        opacityFunc2.AddPoint(60.0, 0.0);
        opacityFunc2.AddPoint(120.0, 0.1);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc3 = vtkPiecewiseFunction()
        opacityFunc3.AddPoint(0.0, 0.0);
        opacityFunc3.AddPoint(120.0, 0.0);
        opacityFunc3.AddPoint(180.0, 0.1);
        opacityFunc3.AddPoint(255.0, 0.0);

        opacityFunc4 = vtkPiecewiseFunction()
        opacityFunc4.AddPoint(0.0, 0.0);
        opacityFunc4.AddPoint(180.0, 0.0);
        opacityFunc4.AddPoint(255.0, 0.1);

        # Color transfer functions
        color1 = vtkColorTransferFunction()
        color1.AddRGBPoint(0.0, 1.0, 0.0, 0.0);
        color1.AddRGBPoint(60.0, 1.0, 0.0, 0.0);

        color2 = vtkColorTransferFunction()
        color2.AddRGBPoint(60.0, 0.0, 0.0, 1.0);
        color2.AddRGBPoint(120.0, 0.0, 0.0, 1.0);

        color3 = vtkColorTransferFunction()
        color3.AddRGBPoint(120.0, 0.0, 1.0, 0.0);
        color3.AddRGBPoint(180.0, 0.0, 1.0, 0.0);

        color4 = vtkColorTransferFunction()
        color4.AddRGBPoint(180.0, 0.0, 0.0, 0.0);
        color4.AddRGBPoint(239.0, 0.0, 0.0, 0.0);

        # Now set the opacity and the color
        volumeProperty = volume.GetProperty()
        volumeProperty.SetIndependentComponents(1)
        volumeProperty.SetScalarOpacity(0, opacityFunc1)
        volumeProperty.SetScalarOpacity(1, opacityFunc2)
        volumeProperty.SetScalarOpacity(2, opacityFunc3)
        volumeProperty.SetScalarOpacity(3, opacityFunc4)
        volumeProperty.SetColor(0, color1)
        volumeProperty.SetColor(1, color2)
        volumeProperty.SetColor(2, color3)
        volumeProperty.SetColor(3, color4)

        iRen.Initialize();
        ren.SetBackground(0.1,0.4,0.2)
        ren.ResetCamera()
        renWin.Render()

        img_file = "TestGPURayCastIndependentComponentMIP.png"
        vtkmodules.test.Testing.compareImage(
          iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()
        #iRen.Start()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(TestGPURayCastIndependentComponentMIP, 'test')])
