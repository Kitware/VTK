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
# vtkpython deciFranFace.py  -D $VTK_DATA_ROOT \
# -B $VTK_DATA_ROOT/Baseline/Graphics

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

class deciFranFace(vtk.test.Testing.vtkTest):

    def testDeciFranFace(self):

        # Create the RenderWindow, Renderer and both Actors
        #
        ren1 = vtk.vtkRenderer()
        ren2 = vtk.vtkRenderer()
        ren3 = vtk.vtkRenderer()
        ren4 = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren1)
        renWin.AddRenderer(ren2)
        renWin.AddRenderer(ren3)
        renWin.AddRenderer(ren4)

        pnm1 = vtk.vtkPNGReader()
        pnm1.SetFileName(VTK_DATA_ROOT + "/Data/fran_cut.png")
        atext = vtk.vtkTexture()
        atext.SetInputConnection(pnm1.GetOutputPort())
        atext.InterpolateOn()

        # create a cyberware source
        #
        fran = vtk.vtkPolyDataReader()
        fran.SetFileName(VTK_DATA_ROOT + "/Data/fran_cut.vtk")

        # Create a table of decimation conditions
        #
        boundaryVertexDeletion = ["On", "Off"]
        accumulates = ["On", "Off"]

        deci = dict()
        mapper = dict()
        actor = dict()

        for topology in boundaryVertexDeletion:
            for accumulate in accumulates:
                idx = topology + accumulate
                deci.update({idx: vtk.vtkDecimatePro()})
                deci[idx].SetInputConnection(fran.GetOutputPort())
                deci[idx].SetTargetReduction(.95)
                if topology == "On":
                    deci[idx].PreserveTopologyOn()
                elif topology == "Off":
                    deci[idx].PreserveTopologyOff()
                if accumulate == "On":
                    deci[idx].AccumulateErrorOn()
                elif accumulate == "Off":
                    deci[idx].AccumulateErrorOff()
                mapper.update({idx: vtk.vtkPolyDataMapper()})
                mapper[idx].SetInputConnection(deci[idx].GetOutputPort())
                actor.update({idx: vtk.vtkActor()})
                actor[idx].SetMapper(mapper[idx])
                actor[idx].SetTexture(atext)

        # Add the actors to the renderer, set the background and size
        #
        ren1.SetViewport(0, .5, .5, 1)
        ren2.SetViewport(.5, .5, 1, 1)
        ren3.SetViewport(0, 0, .5, .5)
        ren4.SetViewport(.5, 0, 1, .5)

        ren1.AddActor(actor["OnOn"])
        ren2.AddActor(actor["OnOff"])
        ren3.AddActor(actor["OffOn"])
        ren4.AddActor(actor["OffOff"])

        camera = vtk.vtkCamera()
        ren1.SetActiveCamera(camera)
        ren2.SetActiveCamera(camera)
        ren3.SetActiveCamera(camera)
        ren4.SetActiveCamera(camera)

        ren1.GetActiveCamera().SetPosition(0.314753, -0.0699988, -0.264225)
        ren1.GetActiveCamera().SetFocalPoint(0.00188636, -0.136847, -5.84226e-09)
        ren1.GetActiveCamera().SetViewAngle(30)
        ren1.GetActiveCamera().SetViewUp(0, 1, 0)
        ren1.ResetCameraClippingRange()

        ren2.ResetCameraClippingRange()
        ren3.ResetCameraClippingRange()
        ren4.ResetCameraClippingRange()

        ren1.SetBackground(1, 1, 1)
        ren2.SetBackground(1, 1, 1)
        ren3.SetBackground(1, 1, 1)
        ren4.SetBackground(1, 1, 1)
        renWin.SetSize(500, 500)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "deciFranFace.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(deciFranFace, 'test')])
