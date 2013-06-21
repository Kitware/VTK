#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class FixedPointRayCasterNearest(object):

    def __init__(self, ren, renWin, iren):
        self.ren = ren
        self.renWin = renWin
        self.iren = iren

        # Create a gaussian
        gs = vtk.vtkImageGaussianSource()
        gs.SetWholeExtent(0, 30, 0, 30, 0, 30)
        gs.SetMaximum(255.0)
        gs.SetStandardDeviation(5)
        gs.SetCenter(15, 15, 15)

        # threshold to leave a gap that should show up for
        # gradient opacity
        t = vtk.vtkImageThreshold()
        t.SetInputConnection(gs.GetOutputPort())
        t.ReplaceInOn()
        t.SetInValue(0)
        t.ThresholdBetween(150, 200)

        # Use a shift scale to convert to unsigned char
        ss = vtk.vtkImageShiftScale()
        ss.SetInputConnection(t.GetOutputPort())
        ss.SetOutputScalarTypeToUnsignedChar()

        # grid will be used for two component dependent
        grid0 = vtk.vtkImageGridSource()
        grid0.SetDataScalarTypeToUnsignedChar()
        grid0.SetGridSpacing(10, 10, 10)
        grid0.SetLineValue(200)
        grid0.SetFillValue(10)
        grid0.SetDataExtent(0, 30, 0, 30, 0, 30)

        # use dilation to thicken the grid
        d = vtk.vtkImageContinuousDilate3D()
        d.SetInputConnection(grid0.GetOutputPort())
        d.SetKernelSize(3, 3, 3)

        # Now make a two component dependent
        iac = vtk.vtkImageAppendComponents()
        iac.AddInputConnection(d.GetOutputPort())
        iac.AddInputConnection(ss.GetOutputPort())

        # Some more gaussians for the four component indepent case
        gs1 = vtk.vtkImageGaussianSource()
        gs1.SetWholeExtent(0, 30, 0, 30, 0, 30)
        gs1.SetMaximum(255.0)
        gs1.SetStandardDeviation(4)
        gs1.SetCenter(5, 5, 5)

        t1 = vtk.vtkImageThreshold()
        t1.SetInputConnection(gs1.GetOutputPort())
        t1.ReplaceInOn()
        t1.SetInValue(0)
        t1.ThresholdBetween(150, 256)

        gs2 = vtk.vtkImageGaussianSource()
        gs2.SetWholeExtent(0, 30, 0, 30, 0, 30)
        gs2.SetMaximum(255.0)
        gs2.SetStandardDeviation(4)
        gs2.SetCenter(12, 12, 12)

        gs3 = vtk.vtkImageGaussianSource()
        gs3.SetWholeExtent(0, 30, 0, 30, 0, 30)
        gs3.SetMaximum(255.0)
        gs3.SetStandardDeviation(4)
        gs3.SetCenter(19, 19, 19)

        t3 = vtk.vtkImageThreshold()
        t3.SetInputConnection(gs3.GetOutputPort())
        t3.ReplaceInOn()
        t3.SetInValue(0)
        t3.ThresholdBetween(150, 256)

        gs4 = vtk.vtkImageGaussianSource()
        gs4.SetWholeExtent(0, 30, 0, 30, 0, 30)
        gs4.SetMaximum(255.0)
        gs4.SetStandardDeviation(4)
        gs4.SetCenter(26, 26, 26)

        # we need a few append filters ...
        iac1 = vtk.vtkImageAppendComponents()
        iac1.AddInputConnection(t1.GetOutputPort())
        iac1.AddInputConnection(gs2.GetOutputPort())

        iac2 = vtk.vtkImageAppendComponents()
        iac2.AddInputConnection(iac1.GetOutputPort())
        iac2.AddInputConnection(t3.GetOutputPort())

        iac3 = vtk.vtkImageAppendComponents()
        iac3.AddInputConnection(iac2.GetOutputPort())
        iac3.AddInputConnection(gs4.GetOutputPort())

        # create the four component dependend -
        # use lines in x, y, z for colors
        gridR = vtk.vtkImageGridSource()
        gridR.SetDataScalarTypeToUnsignedChar()
        gridR.SetGridSpacing(10, 100, 100)
        gridR.SetLineValue(250)
        gridR.SetFillValue(100)
        gridR.SetDataExtent(0, 30, 0, 30, 0, 30)

        dR = vtk.vtkImageContinuousDilate3D()
        dR.SetInputConnection(gridR.GetOutputPort())
        dR.SetKernelSize(2, 2, 2)

        gridG = vtk.vtkImageGridSource()
        gridG.SetDataScalarTypeToUnsignedChar()
        gridG.SetGridSpacing(100, 10, 100)
        gridG.SetLineValue(250)
        gridG.SetFillValue(100)
        gridG.SetDataExtent(0, 30, 0, 30, 0, 30)

        dG = vtk.vtkImageContinuousDilate3D()
        dG.SetInputConnection(gridG.GetOutputPort())
        dG.SetKernelSize(2, 2, 2)

        gridB = vtk.vtkImageGridSource()
        gridB.SetDataScalarTypeToUnsignedChar()
        gridB.SetGridSpacing(100, 100, 10)
        gridB.SetLineValue(0)
        gridB.SetFillValue(250)
        gridB.SetDataExtent(0, 30, 0, 30, 0, 30)

        dB = vtk.vtkImageContinuousDilate3D()
        dB.SetInputConnection(gridB.GetOutputPort())
        dB.SetKernelSize(2, 2, 2)

        # need some appending
        iacRG = vtk.vtkImageAppendComponents()
        iacRG.AddInputConnection(dR.GetOutputPort())
        iacRG.AddInputConnection(dG.GetOutputPort())

        iacRGB = vtk.vtkImageAppendComponents()
        iacRGB.AddInputConnection(iacRG.GetOutputPort())
        iacRGB.AddInputConnection(dB.GetOutputPort())

        iacRGBA = vtk.vtkImageAppendComponents()
        iacRGBA.AddInputConnection(iacRGB.GetOutputPort())
        iacRGBA.AddInputConnection(ss.GetOutputPort())

        # We need a bunch of opacity functions

        # this one is a simple ramp to .2
        rampPoint2 = vtk.vtkPiecewiseFunction()
        rampPoint2.AddPoint(0, 0.0)
        rampPoint2.AddPoint(255, 0.2)

        # this one is a simple ramp to 1
        ramp1 = vtk.vtkPiecewiseFunction()
        ramp1.AddPoint(0, 0.0)
        ramp1.AddPoint(255, 1.0)

        # this one shows a sharp surface
        surface = vtk.vtkPiecewiseFunction()
        surface.AddPoint(0, 0.0)
        surface.AddPoint(10, 0.0)
        surface.AddPoint(50, 1.0)
        surface.AddPoint(255, 1.0)


        # this one is constant 1
        constant1 = vtk.vtkPiecewiseFunction()
        constant1.AddPoint(0, 1.0)
        constant1.AddPoint(255, 1.0)

        # this one is used for gradient opacity
        gop = vtk.vtkPiecewiseFunction()
        gop.AddPoint(0, 0.0)
        gop.AddPoint(20, 0.0)
        gop.AddPoint(60, 1.0)
        gop.AddPoint(255, 1.0)


        # We need a bunch of color functions

        # This one is a simple rainbow
        rainbow = vtk.vtkColorTransferFunction()
        rainbow.SetColorSpaceToHSV()
        rainbow.HSVWrapOff()
        rainbow.AddHSVPoint(0, 0.1, 1.0, 1.0)
        rainbow.AddHSVPoint(255, 0.9, 1.0, 1.0)

        # this is constant red
        red = vtk.vtkColorTransferFunction()
        red.AddRGBPoint(0, 1, 0, 0)
        red.AddRGBPoint(255, 1, 0, 0)

        # this is constant green
        green = vtk.vtkColorTransferFunction()
        green.AddRGBPoint(0, 0, 1, 0)
        green.AddRGBPoint(255, 0, 1, 0)

        # this is constant blue
        blue = vtk.vtkColorTransferFunction()
        blue.AddRGBPoint(0, 0, 0, 1)
        blue.AddRGBPoint(255, 0, 0, 1)

        # this is constant yellow
        yellow = vtk.vtkColorTransferFunction()
        yellow.AddRGBPoint(0, 1, 1, 0)
        yellow.AddRGBPoint(255, 1, 1, 0)


        #ren = vtk.vtkRenderer()
        #renWin = vtk.vtkRenderWindow()
        self.renWin.AddRenderer(self.ren)
        self.renWin.SetSize(500, 500)
        #iren = vtk.vtkRenderWindowInteractor()
        self.iren.SetRenderWindow(self.renWin)

        self.ren.GetCullers().InitTraversal()
        culler = self.ren.GetCullers().GetNextItem()
        culler.SetSortingStyleToBackToFront()

        # We need 25 mapper / actor pairs which we will render
        # in a grid. Going down we will vary the input data
        # with the top row unsigned char, then float, then
        # two dependent components, then four dependent components
        # then four independent components. Going across we
        # will vary the rendering method with MIP, Composite,
        # Composite Shade, Composite GO, and Composite GO Shade.

        # Create the 5 by 5 grids
        self.volumeProperty = [[0 for col in range(0, 5)] for row in range(0, 5)]
        self.volumeMapper = [[0 for col in range(0, 5)] for row in range(0, 5)]
        volume = [[0 for col in range(0, 5)] for row in range(0, 5)]

        for i in range(0, 5):
            for j in range(0, 5):

                self.volumeProperty[i][j] = vtk.vtkVolumeProperty()
                self.volumeMapper[i][j] = vtk.vtkFixedPointVolumeRayCastMapper()
                self.volumeMapper[i][j].SetSampleDistance(0.25)
                self.volumeMapper[i][j].SetNumberOfThreads(1)

                volume[i][j] = vtk.vtkVolume()
                volume[i][j].SetMapper(self.volumeMapper[i][j])
                volume[i][j].SetProperty(self.volumeProperty[i][j])

                volume[i][j].AddPosition(i * 30, j * 30, 0)

                self.ren.AddVolume(volume[i][j])



        for i in range(0, 5):

            self.volumeMapper[0][i].SetInputConnection(t.GetOutputPort())
            self.volumeMapper[1][i].SetInputConnection(ss.GetOutputPort())
            self.volumeMapper[2][i].SetInputConnection(iac.GetOutputPort())
            self.volumeMapper[3][i].SetInputConnection(iac3.GetOutputPort())
            self.volumeMapper[4][i].SetInputConnection(iacRGBA.GetOutputPort())

            self.volumeMapper[i][0].SetBlendModeToMaximumIntensity()
            self.volumeMapper[i][1].SetBlendModeToComposite()
            self.volumeMapper[i][2].SetBlendModeToComposite()
            self.volumeMapper[i][3].SetBlendModeToComposite()
            self.volumeMapper[i][4].SetBlendModeToComposite()

            self.volumeProperty[0][i].IndependentComponentsOn()
            self.volumeProperty[1][i].IndependentComponentsOn()
            self.volumeProperty[2][i].IndependentComponentsOff()
            self.volumeProperty[3][i].IndependentComponentsOn()
            self.volumeProperty[4][i].IndependentComponentsOff()

            self.volumeProperty[0][i].SetColor(rainbow)
            self.volumeProperty[0][i].SetScalarOpacity(rampPoint2)
            self.volumeProperty[0][i].SetGradientOpacity(constant1)

            self.volumeProperty[1][i].SetColor(rainbow)
            self.volumeProperty[1][i].SetScalarOpacity(rampPoint2)
            self.volumeProperty[1][i].SetGradientOpacity(constant1)

            self.volumeProperty[2][i].SetColor(rainbow)
            self.volumeProperty[2][i].SetScalarOpacity(rampPoint2)
            self.volumeProperty[2][i].SetGradientOpacity(constant1)

            self.volumeProperty[3][i].SetColor(0, red)
            self.volumeProperty[3][i].SetColor(1, green)
            self.volumeProperty[3][i].SetColor(2, blue)
            self.volumeProperty[3][i].SetColor(3, yellow)
            self.volumeProperty[3][i].SetScalarOpacity(0, rampPoint2)
            self.volumeProperty[3][i].SetScalarOpacity(1, rampPoint2)
            self.volumeProperty[3][i].SetScalarOpacity(2, rampPoint2)
            self.volumeProperty[3][i].SetScalarOpacity(3, rampPoint2)
            self.volumeProperty[3][i].SetGradientOpacity(0, constant1)
            self.volumeProperty[3][i].SetGradientOpacity(1, constant1)
            self.volumeProperty[3][i].SetGradientOpacity(2, constant1)
            self.volumeProperty[3][i].SetGradientOpacity(3, constant1)
            self.volumeProperty[3][i].SetComponentWeight(0, 1)
            self.volumeProperty[3][i].SetComponentWeight(1, 1)
            self.volumeProperty[3][i].SetComponentWeight(2, 1)
            self.volumeProperty[3][i].SetComponentWeight(3, 1)

            self.volumeProperty[4][i].SetColor(rainbow)
            self.volumeProperty[4][i].SetScalarOpacity(rampPoint2)
            self.volumeProperty[4][i].SetGradientOpacity(constant1)

            self.volumeProperty[i][2].ShadeOn()
            self.volumeProperty[i][4].ShadeOn(0)
            self.volumeProperty[i][4].ShadeOn(1)
            self.volumeProperty[i][4].ShadeOn(2)
            self.volumeProperty[i][4].ShadeOn(3)


        self.volumeProperty[0][0].SetScalarOpacity(ramp1)
        self.volumeProperty[1][0].SetScalarOpacity(ramp1)
        self.volumeProperty[2][0].SetScalarOpacity(ramp1)
        self.volumeProperty[3][0].SetScalarOpacity(0, surface)
        self.volumeProperty[3][0].SetScalarOpacity(1, surface)
        self.volumeProperty[3][0].SetScalarOpacity(2, surface)
        self.volumeProperty[3][0].SetScalarOpacity(3, surface)
        self.volumeProperty[4][0].SetScalarOpacity(ramp1)

        self.volumeProperty[0][2].SetScalarOpacity(surface)
        self.volumeProperty[1][2].SetScalarOpacity(surface)
        self.volumeProperty[2][2].SetScalarOpacity(surface)
        self.volumeProperty[3][2].SetScalarOpacity(0, surface)
        self.volumeProperty[3][2].SetScalarOpacity(1, surface)
        self.volumeProperty[3][2].SetScalarOpacity(2, surface)
        self.volumeProperty[3][2].SetScalarOpacity(3, surface)
        self.volumeProperty[4][2].SetScalarOpacity(surface)

        self.volumeProperty[0][4].SetScalarOpacity(surface)
        self.volumeProperty[1][4].SetScalarOpacity(surface)
        self.volumeProperty[2][4].SetScalarOpacity(surface)
        self.volumeProperty[3][4].SetScalarOpacity(0, surface)
        self.volumeProperty[3][4].SetScalarOpacity(1, surface)
        self.volumeProperty[3][4].SetScalarOpacity(2, surface)
        self.volumeProperty[3][4].SetScalarOpacity(3, surface)
        self.volumeProperty[4][4].SetScalarOpacity(surface)

        self.volumeProperty[0][3].SetGradientOpacity(gop)
        self.volumeProperty[1][3].SetGradientOpacity(gop)
        self.volumeProperty[2][3].SetGradientOpacity(gop)
        self.volumeProperty[3][3].SetGradientOpacity(0, gop)
        self.volumeProperty[3][3].SetGradientOpacity(2, gop)
        self.volumeProperty[4][3].SetGradientOpacity(gop)

        self.volumeProperty[3][3].SetScalarOpacity(0, ramp1)
        self.volumeProperty[3][3].SetScalarOpacity(2, ramp1)

        self.volumeProperty[0][4].SetGradientOpacity(gop)
        self.volumeProperty[1][4].SetGradientOpacity(gop)
        self.volumeProperty[2][4].SetGradientOpacity(gop)
        self.volumeProperty[3][4].SetGradientOpacity(0, gop)
        self.volumeProperty[3][4].SetGradientOpacity(2, gop)
        self.volumeProperty[4][4].SetGradientOpacity(gop)

        self.renWin.Render()

        self.ren.GetActiveCamera().Dolly(1.3)
        self.ren.GetActiveCamera().Azimuth(15)
        self.ren.GetActiveCamera().Elevation(5)
        self.ren.ResetCameraClippingRange()


        #        self.renWin.Render()

    def GetVolumeProperty(self):
        ''' Return the volumeProperty so other tests can use it.'''
        return self.volumeProperty

    def GetVolumeMapper(self):
        ''' Return the volumeMapper so other tests can use it.'''
        return self.volumeMapper

class TestFixedPointRayCasterNearest(vtk.test.Testing.vtkTest):

    def testFixedPointRayCasterNearest(self):
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        iRen = vtk.vtkRenderWindowInteractor()

        tFPRCN = FixedPointRayCasterNearest(ren, renWin, iRen)

        # render and interact with data

        renWin.Render()

        img_file = "TestFixedPointRayCasterNearest.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=10)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(TestFixedPointRayCasterNearest, 'test')])
