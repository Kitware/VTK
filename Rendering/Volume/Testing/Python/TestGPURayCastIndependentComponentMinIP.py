#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastIndependentComponentMinIP.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import sys
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

class TestGPURayCastIndependentComponentMinIP(vtk.test.Testing.vtkTest):

    def test(self):
        dataRoot = vtkGetDataRoot()
        reader =  vtk.vtkXMLImageDataReader()
        reader.SetFileName("" + str(dataRoot) + "/Data/vase_4comp.vti")

        volume = vtk.vtkVolume()
        #mapper = vtk.vtkFixedPointVolumeRayCastMapper()
        mapper = vtk.vtkGPUVolumeRayCastMapper()
        mapper.SetBlendModeToMinimumIntensity();
        mapper.SetSampleDistance(0.1)
        mapper.SetAutoAdjustSampleDistances(0)
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        iRen = vtk.vtkRenderWindowInteractor()

        # Set connections
        mapper.SetInputConnection(reader.GetOutputPort());
        volume.SetMapper(mapper)
        ren.AddViewProp(volume)
        renWin.AddRenderer(ren)
        iRen.SetRenderWindow(renWin)

        # Define opacity transfer function and color functions
        opacityFunc1 = vtk.vtkPiecewiseFunction()
        opacityFunc1.AddPoint(0.0, 0.0);
        opacityFunc1.AddPoint(60.0, 0.1);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc2 = vtk.vtkPiecewiseFunction()
        opacityFunc2.AddPoint(0.0, 0.1);
        opacityFunc2.AddPoint(60.0, 0.0);
        opacityFunc2.AddPoint(120.0, 0.0);
        opacityFunc1.AddPoint(255.0, 0.0);

        opacityFunc3 = vtk.vtkPiecewiseFunction()
        opacityFunc3.AddPoint(0.0, 0.0);
        opacityFunc3.AddPoint(120.0, 0.1);
        opacityFunc3.AddPoint(180.0, 0.0);
        opacityFunc3.AddPoint(255.0, 0.0);

        opacityFunc4 = vtk.vtkPiecewiseFunction()
        opacityFunc4.AddPoint(0.0, 0.0);
        opacityFunc4.AddPoint(180.0, 0.1);
        opacityFunc4.AddPoint(255.0, 0.0);

        # Color transfer functions
        color1 = vtk.vtkColorTransferFunction()
        color1.AddRGBPoint(0.0, 1.0, 0.0, 0.0);
        color1.AddRGBPoint(60.0, 1.0, 0.0, 0.0);

        color2 = vtk.vtkColorTransferFunction()
        color2.AddRGBPoint(60.0, 0.0, 0.0, 1.0);
        color2.AddRGBPoint(120.0, 0.0, 0.0, 1.0);

        color3 = vtk.vtkColorTransferFunction()
        color3.AddRGBPoint(120.0, 0.0, 1.0, 0.0);
        color3.AddRGBPoint(180.0, 0.0, 1.0, 0.0);

        color4 = vtk.vtkColorTransferFunction()
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

        img_file = "TestGPURayCastIndependentComponentMinIP.png"
        vtk.test.Testing.compareImage(
          iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtk.test.Testing.interact()
        #iRen.Start()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestGPURayCastIndependentComponentMinIP, 'test')])
