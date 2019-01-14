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

class VolumePicker(vtk.test.Testing.vtkTest):

    def testVolumePicker(self):
        # volume render a medical data set

        # renderer and interactor
        ren = vtk.vtkRenderer()

        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin)

        # read the volume
        v16 = vtk.vtkVolume16Reader()
        v16.SetDataDimensions(64, 64)
        v16.SetImageRange(1, 93)
        v16.SetDataByteOrderToLittleEndian()
        v16.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
        v16.SetDataSpacing(3.2, 3.2, 1.5)

        #---------------------------------------------------------
        # set up the volume rendering

        volumeMapper = vtk.vtkFixedPointVolumeRayCastMapper()
        volumeMapper.SetInputConnection(v16.GetOutputPort())

        volumeColor = vtk.vtkColorTransferFunction()
        volumeColor.AddRGBPoint(0, 0.0, 0.0, 0.0)
        volumeColor.AddRGBPoint(180, 0.3, 0.1, 0.2)
        volumeColor.AddRGBPoint(1000, 1.0, 0.7, 0.6)
        volumeColor.AddRGBPoint(2000, 1.0, 1.0, 0.9)

        volumeScalarOpacity = vtk.vtkPiecewiseFunction()
        volumeScalarOpacity.AddPoint(0, 0.0)
        volumeScalarOpacity.AddPoint(180, 0.0)
        volumeScalarOpacity.AddPoint(1000, 0.2)
        volumeScalarOpacity.AddPoint(2000, 0.8)

        volumeGradientOpacity = vtk.vtkPiecewiseFunction()
        volumeGradientOpacity.AddPoint(0, 0.0)
        volumeGradientOpacity.AddPoint(90, 0.5)
        volumeGradientOpacity.AddPoint(100, 1.0)

        volumeProperty = vtk.vtkVolumeProperty()
        volumeProperty.SetColor(volumeColor)
        volumeProperty.SetScalarOpacity(volumeScalarOpacity)
        volumeProperty.SetGradientOpacity(volumeGradientOpacity)
        volumeProperty.SetInterpolationTypeToLinear()
        volumeProperty.ShadeOn()
        volumeProperty.SetAmbient(0.6)
        volumeProperty.SetDiffuse(0.6)
        volumeProperty.SetSpecular(0.1)

        volume = vtk.vtkVolume()
        volume.SetMapper(volumeMapper)
        volume.SetProperty(volumeProperty)

        #---------------------------------------------------------
        # Do the surface rendering
        boneExtractor = vtk.vtkMarchingCubes()
        boneExtractor.SetInputConnection(v16.GetOutputPort())
        boneExtractor.SetValue(0, 1150)

        boneNormals = vtk.vtkPolyDataNormals()
        boneNormals.SetInputConnection(boneExtractor.GetOutputPort())
        boneNormals.SetFeatureAngle(60.0)

        boneStripper = vtk.vtkStripper()
        boneStripper.SetInputConnection(boneNormals.GetOutputPort())

        boneMapper = vtk.vtkPolyDataMapper()
        boneMapper.SetInputConnection(boneStripper.GetOutputPort())
        boneMapper.ScalarVisibilityOff()

        boneProperty = vtk.vtkProperty()
        boneProperty.SetColor(1.0, 1.0, 0.9)

        bone = vtk.vtkActor()
        bone.SetMapper(boneMapper)
        bone.SetProperty(boneProperty)

        #---------------------------------------------------------
        # Create an image actor

        table = vtk.vtkLookupTable()
        table.SetRange(0, 2000)
        table.SetRampToLinear()
        table.SetValueRange(0, 1)
        table.SetHueRange(0, 0)
        table.SetSaturationRange(0, 0)

        mapToColors = vtk.vtkImageMapToColors()
        mapToColors.SetInputConnection(v16.GetOutputPort())
        mapToColors.SetLookupTable(table)

        imageActor = vtk.vtkImageActor()
        imageActor.GetMapper().SetInputConnection(mapToColors.GetOutputPort())
        imageActor.SetDisplayExtent(32, 32, 0, 63, 0, 92)

        #---------------------------------------------------------
        # make a transform and some clipping planes

        transform = vtk.vtkTransform()
        transform.RotateWXYZ(-20, 0.0, -0.7, 0.7)

        volume.SetUserTransform(transform)
        bone.SetUserTransform(transform)
        imageActor.SetUserTransform(transform)

        c = volume.GetCenter()

        volumeClip = vtk.vtkPlane()
        volumeClip.SetNormal(0, 1, 0)
        volumeClip.SetOrigin(c)

        boneClip = vtk.vtkPlane()
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
        coneSource = vtk.vtkConeSource()
        coneSource.CappingOn()
        coneSource.SetHeight(12)
        coneSource.SetRadius(5)
        coneSource.SetResolution(31)
        coneSource.SetCenter(6, 0, 0)
        coneSource.SetDirection(-1, 0, 0)

        #---------------------------------------------------------
        picker = vtk.vtkVolumePicker()
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

        coneActor1 = vtk.vtkActor()
        coneActor1.PickableOff()
        coneMapper1 = vtk.vtkDataSetMapper()
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

        coneActor2 = vtk.vtkActor()
        coneActor2.PickableOff()
        coneMapper2 = vtk.vtkDataSetMapper()
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

        coneActor3 = vtk.vtkActor()
        coneActor3.PickableOff()
        coneMapper3 = vtk.vtkDataSetMapper()
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

        coneActor4 = vtk.vtkActor()
        coneActor4.PickableOff()
        coneMapper4 = vtk.vtkDataSetMapper()
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
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(VolumePicker, 'test')])
