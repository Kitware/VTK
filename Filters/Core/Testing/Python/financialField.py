#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkFiltersCore import (
    vtkDataObjectToDataSetFilter,
    vtkFieldDataToAttributeDataFilter,
    vtkTubeFilter,
)
from vtkmodules.vtkFiltersGeneral import (
    vtkAxes,
    vtkMarchingContourFilter,
)
from vtkmodules.vtkIOLegacy import vtkDataObjectReader
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

class financialField(vtkmodules.test.Testing.vtkTest):

    def testFinancialField(self):

        size = 3187 #maximum number possible

        #set size 100 #maximum number possible
        xAxis = "INTEREST_RATE"
        yAxis = "MONTHLY_PAYMENT"
        zAxis = "MONTHLY_INCOME"
        scalar = "TIME_LATE"


        # extract data from field as a polydata (just points), then extract scalars
        fdr = vtkDataObjectReader()
        fdr.SetFileName(VTK_DATA_ROOT + "/Data/financial.vtk")
        do2ds = vtkDataObjectToDataSetFilter()
        do2ds.SetInputConnection(fdr.GetOutputPort())
        do2ds.SetDataSetTypeToPolyData()
        #format: component#, arrayname, arraycomp, minArrayId, maxArrayId, normalize
        do2ds.DefaultNormalizeOn()
        do2ds.SetPointComponent(0, xAxis, 0)
        do2ds.SetPointComponent(1, yAxis, 0, 0, size, 1)
        do2ds.SetPointComponent(2, zAxis, 0)
        do2ds.Update()
        if fdr.GetOutput().GetFieldData().GetAbstractArray("Some Text").GetValue(0) != "Test me":
            raise RuntimeError('Could not properly read string array "Some Text"')
        fd2ad = vtkFieldDataToAttributeDataFilter()
        fd2ad.SetInputConnection(do2ds.GetOutputPort())
        fd2ad.SetInputFieldToDataObjectField()
        fd2ad.SetOutputAttributeDataToPointData()
        fd2ad.DefaultNormalizeOn()
        fd2ad.SetScalarComponent(0, scalar, 0)

        # construct pipeline for original population
        popSplatter = vtkGaussianSplatter()
        popSplatter.SetInputConnection(fd2ad.GetOutputPort())
        popSplatter.SetSampleDimensions(50, 50, 50)
        popSplatter.SetRadius(0.05)
        popSplatter.ScalarWarpingOff()
        popSurface = vtkMarchingContourFilter()
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
        lateSplatter.SetInputConnection(fd2ad.GetOutputPort())
        lateSplatter.SetSampleDimensions(50, 50, 50)
        lateSplatter.SetRadius(0.05)
        lateSplatter.SetScaleFactor(0.05)
        lateSurface = vtkMarchingContourFilter()
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
        renWin.SetSize(400, 400)

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

        img_file = "financialField.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(financialField, 'test')])
