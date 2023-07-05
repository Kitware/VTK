#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkCommonDataModel import vtkFieldData
from vtkmodules.vtkFiltersCore import (
    vtkArrayCalculator,
    vtkAssignAttribute,
    vtkContourFilter,
    vtkDataObjectToDataSetFilter,
    vtkRearrangeFields,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import vtkAxes
from vtkmodules.vtkFiltersSources import vtkProgrammableDataObjectSource
from vtkmodules.vtkImagingHybrid import vtkGaussianSplatter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkFollower,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkRenderingFreeType import vtkVectorText
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class financialField3(vtkmodules.test.Testing.vtkTest):

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
        dos = vtkProgrammableDataObjectSource()

        def parseFile():
            f = open(VTK_DATA_ROOT + "/Data/financial.txt", "r")

            line = f.readline().split()
            # From the size calculate the number of lines.
            numPts = int(line[1])
            numLines = (numPts - 1) // 8 + 1

            # create the data object
            field = vtkFieldData()
            field.AllocateArrays(4)

            # read TIME_LATE - dependent variable
            while True:
                line = f.readline().split()
                if len(line) > 0:
                    break;
            timeLate = vtkFloatArray()
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
            monthlyPayment = vtkFloatArray()
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
            interestRate = vtkFloatArray()
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
            monthlyIncome = vtkFloatArray()
            monthlyIncome.SetName(line[0])
            for i in range(0, numLines):
                line = f.readline().split()
                for j in line:
                    monthlyIncome.InsertNextValue(float(j))
            field.AddArray(monthlyIncome)

            dos.GetOutput().SetFieldData(field)

        dos.SetExecuteMethod(parseFile)


        # Create the dataset
        do2ds = vtkDataObjectToDataSetFilter()
        do2ds.SetInputConnection(dos.GetOutputPort())
        do2ds.SetDataSetTypeToPolyData()
        #format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
        do2ds.DefaultNormalizeOn()
        do2ds.SetPointComponent(0, xAxis, 0)
        do2ds.SetPointComponent(1, yAxis, 0)
        do2ds.SetPointComponent(2, zAxis, 0)
        do2ds.Update()

        rf = vtkRearrangeFields()
        rf.SetInputConnection(do2ds.GetOutputPort())
        rf.AddOperation("MOVE", scalar, "DATA_OBJECT", "POINT_DATA")
        rf.RemoveOperation("MOVE", scalar, "DATA_OBJECT", "POINT_DATA")
        rf.AddOperation("MOVE", scalar, "DATA_OBJECT", "POINT_DATA")
        rf.RemoveAllOperations()
        rf.AddOperation("MOVE", scalar, "DATA_OBJECT", "POINT_DATA")
        rf.Update()
        max = rf.GetOutput().GetPointData().GetArray(scalar).GetRange(0)[1]


        calc = vtkArrayCalculator()
        calc.SetInputConnection(rf.GetOutputPort())
        calc.SetAttributeTypeToPointData()
        calc.SetFunction("s / %f" % max)
        calc.AddScalarVariable("s", scalar, 0)
        calc.SetResultArrayName("resArray")

        aa = vtkAssignAttribute()
        aa.SetInputConnection(calc.GetOutputPort())
        aa.Assign("resArray", "SCALARS", "POINT_DATA")
        aa.Update()

        rf2 = vtkRearrangeFields()
        rf2.SetInputConnection(aa.GetOutputPort())
        rf2.AddOperation("COPY", "SCALARS", "POINT_DATA", "DATA_OBJECT")

        # construct pipeline for original population
        popSplatter = vtkGaussianSplatter()
        popSplatter.SetInputConnection(rf2.GetOutputPort())
        popSplatter.SetSampleDimensions(50, 50, 50)
        popSplatter.SetRadius(0.05)
        popSplatter.ScalarWarpingOff()
        popSurface = vtkContourFilter()
        popSurface.SetInputConnection(popSplatter.GetOutputPort())
        popSurface.SetValue(0, 0.01)
        popMapper = vtkPolyDataMapper()
        popMapper.SetInputConnection(popSurface.GetOutputPort())
        popMapper.ScalarVisibilityOff()
        popActor = vtkActor()
        popActor.SetMapper(popMapper)
        popActor.GetProperty().SetOpacity(0.3)
        popActor.GetProperty().SetColor(.9, .9, .9)

        # construct pipeline for delinquent population
        lateSplatter = vtkGaussianSplatter()
        lateSplatter.SetInputConnection(aa.GetOutputPort())
        lateSplatter.SetSampleDimensions(50, 50, 50)
        lateSplatter.SetRadius(0.05)
        lateSplatter.SetScaleFactor(0.05)
        lateSurface = vtkContourFilter()
        lateSurface.SetInputConnection(lateSplatter.GetOutputPort())
        lateSurface.SetValue(0, 0.01)
        lateMapper = vtkPolyDataMapper()
        lateMapper.SetInputConnection(lateSurface.GetOutputPort())
        lateMapper.ScalarVisibilityOff()
        lateActor = vtkActor()
        lateActor.SetMapper(lateMapper)
        lateActor.GetProperty().SetColor(1.0, 0.0, 0.0)

        # create axes
        popSplatter.Update()
        bounds = popSplatter.GetOutput().GetBounds()
        axes = vtkAxes()
        axes.SetOrigin(bounds[0], bounds[2], bounds[4])
        axes.SetScaleFactor(popSplatter.GetOutput().GetLength() / 5.0)
        axesTubes = vtkTubeFilter()
        axesTubes.SetInputConnection(axes.GetOutputPort())
        axesTubes.SetRadius(axes.GetScaleFactor() / 25.0)
        axesTubes.SetNumberOfSides(6)
        axesMapper = vtkPolyDataMapper()
        axesMapper.SetInputConnection(axesTubes.GetOutputPort())
        axesActor = vtkActor()
        axesActor.SetMapper(axesMapper)

        # label the axes
        XText = vtkVectorText()
        XText.SetText(xAxis)
        XTextMapper = vtkPolyDataMapper()
        XTextMapper.SetInputConnection(XText.GetOutputPort())
        XActor = vtkFollower()
        XActor.SetMapper(XTextMapper)
        XActor.SetScale(0.02, .02, .02)
        XActor.SetPosition(0.35, -0.05, -0.05)
        XActor.GetProperty().SetColor(0, 0, 0)

        YText = vtkVectorText()
        YText.SetText(yAxis)
        YTextMapper = vtkPolyDataMapper()
        YTextMapper.SetInputConnection(YText.GetOutputPort())
        YActor = vtkFollower()
        YActor.SetMapper(YTextMapper)
        YActor.SetScale(0.02, .02, .02)
        YActor.SetPosition(-0.05, 0.35, -0.05)
        YActor.GetProperty().SetColor(0, 0, 0)

        ZText = vtkVectorText()
        ZText.SetText(zAxis)
        ZTextMapper = vtkPolyDataMapper()
        ZTextMapper.SetInputConnection(ZText.GetOutputPort())
        ZActor = vtkFollower()
        ZActor.SetMapper(ZTextMapper)
        ZActor.SetScale(0.02, .02, .02)
        ZActor.SetPosition(-0.05, -0.05, 0.35)
        ZActor.GetProperty().SetColor(0, 0, 0)

        # Graphics stuff
        #
        ren = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)
        renWin.SetWindowName("vtk(-, Field.Data")
        renWin.SetSize(300, 300)

        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(axesActor)
        ren.AddActor(lateActor)
        ren.AddActor(XActor)
        ren.AddActor(YActor)
        ren.AddActor(ZActor)
        ren.AddActor(popActor) #it's last because its translucent)
        ren.SetBackground(1, 1, 1)

        camera = vtkCamera()
        camera.SetClippingRange(.274, 13.72)
        camera.SetFocalPoint(0.433816, 0.333131, 0.449)
        camera.SetPosition(-1.96987, 1.15145, 1.49053)
        camera.SetViewUp(0.378927, 0.911821, 0.158107)
        ren.SetActiveCamera(camera)
        XActor.SetCamera(camera)
        YActor.SetCamera(camera)
        ZActor.SetCamera(camera)

        # render and interact with data

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "financialField3.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(financialField3, 'test')])
