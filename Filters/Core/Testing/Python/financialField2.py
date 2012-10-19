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

# Run this test like so:
# vtkpython financialField2.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Hybrid

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class financialField2(vtk.test.Testing.vtkTest):

    def testFinancialField(self):

        """
            Demonstrate the use and manipulation of fields and use of
            vtkProgrammableDataObjectSource. This creates fields the hard way
            (as compared to reading a vtk field file), but shows you how to
            interface to your own raw data.

            The image should be the same as financialField.tcl
        """
        xAxis = "INTEREST_RATE"
        yAxis = "MONTHLY_PAYMENT"
        zAxis = "MONTHLY_INCOME"
        scalar = "TIME_LATE"

        # Parse an ascii file and manually create a field. Then construct a
        # dataset from the field.
        dos = vtk.vtkProgrammableDataObjectSource()

        def parseFile():
            f = open(VTK_DATA_ROOT + "/Data/financial.txt", "r")

            line = f.readline().split()
            # From the size calculate the number of lines.
            numPts = int(line[1])
            numLines = (numPts - 1) / 8 + 1

            # create the data object
            field = vtk.vtkFieldData()
            field.AllocateArrays(4)

            # read TIME_LATE - dependent variable
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            timeLate = vtk.vtkFloatArray()
            timeLate.SetName(line[0])
            for i in range(0, numLines):
                line = f.readline().split()
                for j in line:
                    timeLate.InsertNextValue(float(j))
            field.AddArray(timeLate)

            # MONTHLY_PAYMENT - independent variable
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            monthlyPayment = vtk.vtkFloatArray()
            monthlyPayment.SetName(line[0])
            for i in range(0, numLines):
                line = f.readline().split()
                for j in line:
                    monthlyPayment.InsertNextValue(float(j))
            field.AddArray(monthlyPayment)

            # UNPAID_PRINCIPLE - skip
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            for i in range(0, numLines):
                line = f.readline()

            # LOAN_AMOUNT - skip
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            for i in range(0, numLines):
                line = f.readline()

            # INTEREST_RATE - independent variable
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            interestRate = vtk.vtkFloatArray()
            interestRate.SetName(line[0])
            for i in range(0, numLines):
                line = f.readline().split()
                for j in line:
                    interestRate.InsertNextValue(float(j))
            field.AddArray(interestRate)

            # MONTHLY_INCOME - independent variable
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            monthlyIncome = vtk.vtkFloatArray()
            monthlyIncome.SetName(line[0])
            for i in range(0, numLines):
                line = f.readline().split()
                for j in line:
                    monthlyIncome.InsertNextValue(float(j))
            field.AddArray(monthlyIncome)

            dos.GetOutput().SetFieldData(field)

        dos.SetExecuteMethod(parseFile)

        # Create the dataset
        do2ds = vtk.vtkDataObjectToDataSetFilter()
        do2ds.SetInputConnection(dos.GetOutputPort())
        do2ds.SetDataSetTypeToPolyData()
        #format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
        do2ds.DefaultNormalizeOn()
        do2ds.SetPointComponent(0, xAxis, 0)
        do2ds.SetPointComponent(1, yAxis, 0)
        do2ds.SetPointComponent(2, zAxis, 0)
        do2ds.Update()
        fd2ad = vtk.vtkFieldDataToAttributeDataFilter()
        fd2ad.SetInputConnection(do2ds.GetOutputPort())
        fd2ad.SetInputFieldToDataObjectField()
        fd2ad.SetOutputAttributeDataToPointData()
        fd2ad.DefaultNormalizeOn()
        fd2ad.SetScalarComponent(0, scalar, 0)

        # construct pipeline for original population
        popSplatter = vtk.vtkGaussianSplatter()
        popSplatter.SetInputConnection(fd2ad.GetOutputPort())
        popSplatter.SetSampleDimensions(50, 50, 50)
        popSplatter.SetRadius(0.05)
        popSplatter.ScalarWarpingOff()
        popSurface = vtk.vtkMarchingContourFilter()
        popSurface.SetInputConnection(popSplatter.GetOutputPort())
        popSurface.SetValue(0, 0.01)
        popMapper = vtk.vtkPolyDataMapper()
        popMapper.SetInputConnection(popSurface.GetOutputPort())
        popMapper.ScalarVisibilityOff()
        popActor = vtk.vtkActor()
        popActor.SetMapper(popMapper)
        popActor.GetProperty().SetOpacity(0.3)
        popActor.GetProperty().SetColor(.9, .9, .9)

        # construct pipeline for delinquent population
        lateSplatter = vtk.vtkGaussianSplatter()
        lateSplatter.SetInputConnection(fd2ad.GetOutputPort())
        lateSplatter.SetSampleDimensions(50, 50, 50)
        lateSplatter.SetRadius(0.05)
        lateSplatter.SetScaleFactor(0.05)
        lateSurface = vtk.vtkMarchingContourFilter()
        lateSurface.SetInputConnection(lateSplatter.GetOutputPort())
        lateSurface.SetValue(0, 0.01)
        lateMapper = vtk.vtkPolyDataMapper()
        lateMapper.SetInputConnection(lateSurface.GetOutputPort())
        lateMapper.ScalarVisibilityOff()
        lateActor = vtk.vtkActor()
        lateActor.SetMapper(lateMapper)
        lateActor.GetProperty().SetColor(1.0, 0.0, 0.0)

        # create axes
        popSplatter.Update()
        bounds = popSplatter.GetOutput().GetBounds()
        axes = vtk.vtkAxes()
        axes.SetOrigin(bounds[0], bounds[2], bounds[4])
        axes.SetScaleFactor(popSplatter.GetOutput().GetLength() / 5.0)
        axesTubes = vtk.vtkTubeFilter()
        axesTubes.SetInputConnection(axes.GetOutputPort())
        axesTubes.SetRadius(axes.GetScaleFactor() / 25.0)
        axesTubes.SetNumberOfSides(6)
        axesMapper = vtk.vtkPolyDataMapper()
        axesMapper.SetInputConnection(axesTubes.GetOutputPort())
        axesActor = vtk.vtkActor()
        axesActor.SetMapper(axesMapper)

        # label the axes
        XText = vtk.vtkVectorText()
        XText.SetText(xAxis)
        XTextMapper = vtk.vtkPolyDataMapper()
        XTextMapper.SetInputConnection(XText.GetOutputPort())
        XActor = vtk.vtkFollower()
        XActor.SetMapper(XTextMapper)
        XActor.SetScale(0.02, .02, .02)
        XActor.SetPosition(0.35, -0.05, -0.05)
        XActor.GetProperty().SetColor(0, 0, 0)

        YText = vtk.vtkVectorText()
        YText.SetText(yAxis)
        YTextMapper = vtk.vtkPolyDataMapper()
        YTextMapper.SetInputConnection(YText.GetOutputPort())
        YActor = vtk.vtkFollower()
        YActor.SetMapper(YTextMapper)
        YActor.SetScale(0.02, .02, .02)
        YActor.SetPosition(-0.05, 0.35, -0.05)
        YActor.GetProperty().SetColor(0, 0, 0)

        ZText = vtk.vtkVectorText()
        ZText.SetText(zAxis)
        ZTextMapper = vtk.vtkPolyDataMapper()
        ZTextMapper.SetInputConnection(ZText.GetOutputPort())
        ZActor = vtk.vtkFollower()
        ZActor.SetMapper(ZTextMapper)
        ZActor.SetScale(0.02, .02, .02)
        ZActor.SetPosition(-0.05, -0.05, 0.35)
        ZActor.GetProperty().SetColor(0, 0, 0)

        # Graphics stuff
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)
        renWin.SetWindowName("vtk - Field.Data")

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(axesActor)
        ren.AddActor(lateActor)
        ren.AddActor(XActor)
        ren.AddActor(YActor)
        ren.AddActor(ZActor)
        ren.AddActor(popActor) #it's last because its translucent)
        ren.SetBackground(1, 1, 1)
        renWin.SetSize(500, 500)

        camera = vtk.vtkCamera()
        camera.SetClippingRange(.274, 13.72)
        camera.SetFocalPoint(0.433816, 0.333131, 0.449)
        camera.SetPosition(-1.96987, 1.15145, 1.49053)
        camera.SetViewUp(0.378927, 0.911821, 0.158107)
        ren.SetActiveCamera(camera)
        XActor.SetCamera(camera)
        YActor.SetCamera(camera)
        ZActor.SetCamera(camera)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "financialField2.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(financialField2, 'test')])
