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
# vtkpython spatialRepAll.pyy  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Graphics

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class spatialRepAll(vtk.test.Testing.vtkTest):

    def testspatialRepAll(self):

        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        asource = vtk.vtkSTLReader()
        asource.SetFileName(VTK_DATA_ROOT + "/Data/42400-IDGH.stl")
        dataMapper = vtk.vtkPolyDataMapper()
        dataMapper.SetInputConnection(asource.GetOutputPort())
        model = vtk.vtkActor()
        model.SetMapper(dataMapper)
        model.GetProperty().SetColor(1, 0, 0)
        model.VisibilityOn()

        locators = ["vtkPointLocator", "vtkCellLocator", "vtkOBBTree"]

        locator = list()
        boxes = list()
        boxMapper = list()
        boxActor = list()

        for idx, vtkLocatorType in enumerate(locators):
            eval('locator.append(vtk.' + vtkLocatorType + '())')
            locator[idx].AutomaticOff()
            locator[idx].SetMaxLevel(3)

            boxes.append(vtk.vtk.vtkSpatialRepresentationFilter())
            boxes[idx].SetInputConnection(asource.GetOutputPort())
            boxes[idx].SetSpatialRepresentation(locator[idx])
            boxes[idx].SetGenerateLeaves(1)
            boxes[idx].Update()

            output = boxes[idx].GetOutput().GetBlock(boxes[idx].GetMaximumLevel() + 1)

            boxMapper.append(vtk.vtkPolyDataMapper())
            boxMapper[idx].SetInputData(output)

            boxActor.append(vtk.vtkActor())
            boxActor[idx].SetMapper(boxMapper[idx])
            boxActor[idx].AddPosition((idx + 1) * 15, 0, 0)

            ren.AddActor(boxActor[idx])


        ren.AddActor(model)
        ren.SetBackground(0.1, 0.2, 0.4)
        renWin.SetSize(400, 160)

        # render the image
        camera = vtk.vtkCamera()
        camera.SetPosition(148.579, 136.352, 214.961)
        camera.SetFocalPoint(151.889, 86.3178, 223.333)
        camera.SetViewAngle(30)
        camera.SetViewUp(0, 0, -1)
        camera.SetClippingRange(1, 100)
        ren.SetActiveCamera(camera)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "spatialRepAll.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(spatialRepAll, 'test')])
