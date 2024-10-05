#!/usr/bin/env python
# -*- coding: utf-8 -*-



from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import (
    vtkPiecewiseFunction,
    vtkPlane,
)
from vtkmodules.vtkCommonTransforms import vtkTransform
from vtkmodules.vtkFiltersCore import (
    vtkMarchingCubes,
    vtkPolyDataNormals,
    vtkStripper,
)
from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkIOImage import vtkVolume16Reader
from vtkmodules.vtkImagingCore import vtkImageMapToColors
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkColorTransferFunction,
    vtkDataSetMapper,
    vtkImageSlice,
    vtkImageSliceMapper,
    vtkPolyDataMapper,
    vtkProperty,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
    vtkVolume,
    vtkVolumeProperty,
)
from vtkmodules.vtkRenderingVolume import (
    vtkFixedPointVolumeRayCastMapper,
    vtkVolumePicker,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.vtkRenderingVolumeOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class VolumePicker(vtkmodules.test.Testing.vtkTest):

    def testVolumePicker(self):
        # volume render a medical data set

        # renderer and interactor
        ren = vtkRenderer()

        renWin = vtkRenderWindow()
        renWin.AddRenderer(ren)

        iRen = vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        # read the volume
        v16 = vtkVolume16Reader()
        v16.SetDataDimensions(64, 64)
        v16.SetImageRange(1, 93)
        v16.SetDataByteOrderToLittleEndian()
        v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
        v16.SetDataSpacing(3.2, 3.2, 1.5)

        #---------------------------------------------------------
        # set up the volume rendering

        volumeMapper = vtkFixedPointVolumeRayCastMapper()
        volumeMapper.SetInputConnection(v16.GetOutputPort())

        volumeColor = vtkColorTransferFunction()
        volumeColor.AddRGBPoint(0, 0.0, 0.0, 0.0)
        volumeColor.AddRGBPoint(180, 0.3, 0.1, 0.2)
        volumeColor.AddRGBPoint(1000, 1.0, 0.7, 0.6)
        volumeColor.AddRGBPoint(2000, 1.0, 1.0, 0.9)

        volumeScalarOpacity = vtkPiecewiseFunction()
        volumeScalarOpacity.AddPoint(0, 0.0)
        volumeScalarOpacity.AddPoint(180, 0.0)
        volumeScalarOpacity.AddPoint(1000, 0.2)
        volumeScalarOpacity.AddPoint(2000, 0.8)

        volumeGradientOpacity = vtkPiecewiseFunction()
        volumeGradientOpacity.AddPoint(0, 0.0)
        volumeGradientOpacity.AddPoint(90, 0.5)
        volumeGradientOpacity.AddPoint(100, 1.0)

        volumeProperty = vtkVolumeProperty()
        volumeProperty.SetColor(volumeColor)
        volumeProperty.SetScalarOpacity(volumeScalarOpacity)
        volumeProperty.SetGradientOpacity(volumeGradientOpacity)
        volumeProperty.SetInterpolationTypeToLinear()
        volumeProperty.ShadeOn()
        volumeProperty.SetAmbient(0.6)
        volumeProperty.SetDiffuse(0.6)
        volumeProperty.SetSpecular(0.1)

        volume = vtkVolume()
        volume.SetMapper(volumeMapper)
        volume.SetProperty(volumeProperty)

        #---------------------------------------------------------
        # Do the surface rendering
        boneExtractor = vtkMarchingCubes()
        boneExtractor.SetInputConnection(v16.GetOutputPort())
        boneExtractor.SetValue(0, 1150)

        boneNormals = vtkPolyDataNormals()
        boneNormals.SetInputConnection(boneExtractor.GetOutputPort())
        boneNormals.SetFeatureAngle(60.0)

        boneStripper = vtkStripper()
        boneStripper.SetInputConnection(boneNormals.GetOutputPort())

        boneMapper = vtkPolyDataMapper()
        boneMapper.SetInputConnection(boneStripper.GetOutputPort())
        boneMapper.ScalarVisibilityOff()

        boneProperty = vtkProperty()
        boneProperty.SetColor(1.0, 1.0, 0.9)

        bone = vtkActor()
        bone.SetMapper(boneMapper)
        bone.SetProperty(boneProperty)

        #---------------------------------------------------------
        # Create an image actor

        table = vtkLookupTable()
        table.SetRange(0, 2000)
        table.SetRampToLinear()
        table.SetValueRange(0, 1)
        table.SetHueRange(0, 0)
        table.SetSaturationRange(0, 0)

        mapToColors = vtkImageMapToColors()
        mapToColors.SetInputConnection(v16.GetOutputPort())
        mapToColors.SetLookupTable(table)

        imageMapper = vtkImageSliceMapper()
        imageMapper.SetInputConnection(mapToColors.GetOutputPort())
        imageMapper.SetOrientationToX()
        imageMapper.SetSliceNumber(32)

        imageActor = vtkImageSlice()
        imageActor.SetMapper(imageMapper)

        #---------------------------------------------------------
        # make a transform and some clipping planes

        transform = vtkTransform()
        transform.RotateWXYZ(-20, 0.0, -0.7, 0.7)

        volume.SetUserTransform(transform)
        bone.SetUserTransform(transform)
        imageActor.SetUserTransform(transform)

        c = volume.GetCenter()

        volumeClip = vtkPlane()
        volumeClip.SetNormal(0, 1, 0)
        volumeClip.SetOrigin(c)

        boneClip = vtkPlane()
        boneClip.SetNormal(0, 0, 1)
        boneClip.SetOrigin(c)

        volumeMapper.AddClippingPlane(volumeClip)
        boneMapper.AddClippingPlane(boneClip)

        #---------------------------------------------------------
        ren.AddViewProp(volume)
        ren.AddViewProp(bone)
        ren.AddViewProp(imageActor)

        camera = ren.GetActiveCamera()
        camera.SetFocalPoint(c)
        camera.SetPosition(c[0] + 500, c[1] - 100, c[2] - 100)
        camera.SetViewUp(0, 0, -1)

        ren.ResetCameraClippingRange()

        renWin.Render()

        #---------------------------------------------------------
        # the cone should point along the Z axis
        coneSource = vtkConeSource()
        coneSource.CappingOn()
        coneSource.SetHeight(12)
        coneSource.SetRadius(5)
        coneSource.SetResolution(31)
        coneSource.SetCenter(6, 0, 0)
        coneSource.SetDirection(-1, 0, 0)

        #---------------------------------------------------------
        picker = vtkVolumePicker()
        picker.SetTolerance(1.0e-6)
        picker.SetVolumeOpacityIsovalue(0.01)
        # This should usually be left alone, but is used here to increase coverage
        picker.UseVolumeGradientOpacityOn()

        # A function to point an actor along a vector
        def PointCone(actor, n):
            if n[0] < 0.0:
                actor.RotateWXYZ(180, 0, 1, 0)
                actor.RotateWXYZ(180, (n[0] - 1.0) * 0.5, n[1] * 0.5, n[2] * 0.5)
            else:
                actor.RotateWXYZ(180, (n[0] + 1.0) * 0.5, n[1] * 0.5, n[2] * 0.5)

        # Pick the actor
        picker.Pick(192, 103, 0, ren)
        #print picker
        p = picker.GetPickPosition()
        n = picker.GetPickNormal()

        coneActor1 = vtkActor()
        coneActor1.PickableOff()
        coneMapper1 = vtkDataSetMapper()
        coneMapper1.SetInputConnection(coneSource.GetOutputPort())
        coneActor1.SetMapper(coneMapper1)
        coneActor1.GetProperty().SetColor(1, 0, 0)
        coneActor1.SetPosition(p)
        PointCone(coneActor1, n)
        ren.AddViewProp(coneActor1)

        # Pick the volume
        picker.Pick(90, 180, 0, ren)
        p = picker.GetPickPosition()
        n = picker.GetPickNormal()

        coneActor2 = vtkActor()
        coneActor2.PickableOff()
        coneMapper2 = vtkDataSetMapper()
        coneMapper2.SetInputConnection(coneSource.GetOutputPort())
        coneActor2.SetMapper(coneMapper2)
        coneActor2.GetProperty().SetColor(1, 0, 0)
        coneActor2.SetPosition(p)
        PointCone(coneActor2, n)
        ren.AddViewProp(coneActor2)

        # Pick the image
        picker.Pick(200, 200, 0, ren)
        p = picker.GetPickPosition()
        n = picker.GetPickNormal()

        coneActor3 = vtkActor()
        coneActor3.PickableOff()
        coneMapper3 = vtkDataSetMapper()
        coneMapper3.SetInputConnection(coneSource.GetOutputPort())
        coneActor3.SetMapper(coneMapper3)
        coneActor3.GetProperty().SetColor(1, 0, 0)
        coneActor3.SetPosition(p)
        PointCone(coneActor3, n)
        ren.AddViewProp(coneActor3)

        # Pick a clipping plane
        picker.PickClippingPlanesOn()
        picker.Pick(145, 160, 0, ren)
        p = picker.GetPickPosition()
        n = picker.GetPickNormal()

        coneActor4 = vtkActor()
        coneActor4.PickableOff()
        coneMapper4 = vtkDataSetMapper()
        coneMapper4.SetInputConnection(coneSource.GetOutputPort())
        coneActor4.SetMapper(coneMapper4)
        coneActor4.GetProperty().SetColor(1, 0, 0)
        coneActor4.SetPosition(p)
        PointCone(coneActor4, n)
        ren.AddViewProp(coneActor4)

        ren.ResetCameraClippingRange()

        # render and interact with data

        renWin.Render()

        img_file = "VolumePicker.png"
        vtkmodules.test.Testing.compareImage(iRen.GetRenderWindow(), vtkmodules.test.Testing.getAbsImagePath(img_file))
        vtkmodules.test.Testing.interact()

if __name__ == "__main__":
     vtkmodules.test.Testing.main([(VolumePicker, 'test')])
