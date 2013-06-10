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

class contour2DAll(vtk.test.Testing.vtkTest):

    def testContour2DAll(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.SetMultiSamples(0)
        renWin.AddRenderer(ren)

        # create pipeline
        #
        slc = vtk.vtkSLCReader()
        slc.SetFileName(VTK_DATA_ROOT + "/Data/nut.slc")
        slc.Update()

        types = ["Char", "UnsignedChar", "Short", "UnsignedShort", "Int", "UnsignedInt", "Long", "UnsignedLong", "Float", "Double"]

        i = 3

        clip = list()
        cast = list()
        iso = list()
        mapper = list()
        actor = list()

        for idx, vtkType in enumerate(types):

            clip.append(vtk.vtkImageClip())
            clip[idx].SetInputConnection(slc.GetOutputPort())
            clip[idx].SetOutputWholeExtent(-1000, 1000, -1000, 1000, i, i)

            i += 2

            cast.append(vtk.vtkImageCast())
            eval("cast[idx].SetOutputScalarTypeTo" + vtkType)
            cast[idx].SetInputConnection(clip[idx].GetOutputPort())
            cast[idx].ClampOverflowOn()

            iso.append(vtk.vtkContourFilter())
            iso[idx] = vtk.vtkContourFilter()
            iso[idx].SetInputConnection(cast[idx].GetOutputPort())
            iso[idx].GenerateValues(1, 30, 30)

            mapper.append(vtk.vtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())
            mapper[idx].SetColorModeToMapScalars()

            actor.append(vtk.vtkActor())
            actor[idx].SetMapper(mapper[idx])
            ren.AddActor(actor[idx])

        outline = vtk.vtkOutlineFilter()
        outline.SetInputConnection(slc.GetOutputPort())
        outlineMapper = vtk.vtkPolyDataMapper()
        outlineMapper.SetInputConnection(outline.GetOutputPort())
        outlineActor = vtk.vtkActor()
        outlineActor.SetMapper(outlineMapper)
        outlineActor.VisibilityOff()
        #
        # Add the actors to the renderer, set the background and size
        #
        ren.AddActor(outlineActor)
        ren.ResetCamera()
        ren.GetActiveCamera().SetViewAngle(30)
        ren.GetActiveCamera().Elevation(20)
        ren.GetActiveCamera().Azimuth(20)
        ren.GetActiveCamera().Zoom(1.5)
        ren.ResetCameraClippingRange()

        ren.SetBackground(0.9, .9, .9)
        renWin.SetSize(200, 200)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "contour2DAll.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(contour2DAll, 'test')])
