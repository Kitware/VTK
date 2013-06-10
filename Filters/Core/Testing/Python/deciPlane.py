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

class deciPlane(vtk.test.Testing.vtkTest):

    def testDeciPlane(self):

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

        # Create the data -- a plane with a couple of bumps
        #
        plane = vtk.vtkPlaneSource()
        plane.SetXResolution(10)
        plane.SetYResolution(10)

        tf = vtk.vtkTriangleFilter()
        tf.SetInputConnection(plane.GetOutputPort())
        tf.Update()

        def adjustPointsProc():
            input = adjustPoints.GetPolyDataInput()
            inPts = input.GetPoints()
            numPts = input.GetNumberOfPoints()
            newPts = vtk.vtkPoints()
            newPts.SetNumberOfPoints(numPts)

            for i in range(0, numPts):
                newPts.SetPoint(i, inPts.GetPoint(i))

            pt = input.GetPoint(17)
            newPts.SetPoint(17, pt[0], pt[1], 0.25)
            pt = inPts.GetPoint(50)
            newPts.SetPoint(50, pt[0], pt[1], 1.0)
            pt = inPts.GetPoint(77)
            newPts.SetPoint(77, pt[0], pt[1], 0.125)

            adjustPoints.GetPolyDataOutput().CopyStructure(input)
            adjustPoints.GetPolyDataOutput().SetPoints(newPts)


        # This filter modifies the point coordinates in a couple of spots
        #
        adjustPoints = vtk.vtkProgrammableFilter()
        adjustPoints.SetInputConnection(tf.GetOutputPort())
        # The SetExecuteMethod takes a python procedure as an argument
        # In here is where all the processing is done.
        adjustPoints.SetExecuteMethod(adjustPointsProc())
        #adjustPoints.Update()

        # Now remove the extreme peak in the center
        gf = vtk.vtkGeometryFilter()
        gf.SetInputData(adjustPoints.GetPolyDataOutput())
        gf.ExtentClippingOn()
        gf.SetExtent(-100, 100, -100, 100, -1, 0.9)

        # Create a table of decimation conditions
        #
        boundaryVertexDeletion = ["On", "Off"]
        accumulates = ["On", "Off"]

        deci = dict()
        mapper = dict()
        plane = dict()

        #sz = len(boundaryVertexDeletion)
        for topology in boundaryVertexDeletion:
            for accumulate in accumulates:
        #        idx = i * sz + j
                idx = topology + accumulate
                deci.update({idx: vtk.vtkDecimatePro()})
                deci[idx].SetInputConnection(gf.GetOutputPort())
                deci[idx].SetTargetReduction(.95)
                if topology == "On":
                    deci[idx].BoundaryVertexDeletionOn()
                elif topology == "Off":
                    deci[idx].BoundaryVertexDeletionOff()
                if accumulate == "On":
                    deci[idx].AccumulateErrorOn()
                elif accumulate == "Off":
                    deci[idx].AccumulateErrorOff()
                mapper.update({idx: vtk.vtkPolyDataMapper()})
                mapper[idx].SetInputConnection(deci[idx].GetOutputPort())
                plane.update({idx: vtk.vtkActor()})
                plane[idx].SetMapper(mapper[idx])

        # Add the actors to the renderer, set the background and size
        #
        ren1.SetViewport(0, .5, .5, 1)
        ren2.SetViewport(.5, .5, 1, 1)
        ren3.SetViewport(0, 0, .5, .5)
        ren4.SetViewport(.5, 0, 1, .5)

        ren1.AddActor(plane["OnOn"])
        ren2.AddActor(plane["OnOff"])
        ren3.AddActor(plane["OffOn"])
        ren4.AddActor(plane["OffOff"])

        camera = vtk.vtkCamera()
        ren1.SetActiveCamera(camera)
        ren2.SetActiveCamera(camera)
        ren3.SetActiveCamera(camera)
        ren4.SetActiveCamera(camera)

        ren1.GetActiveCamera().SetPosition(-0.128224, 0.611836, 2.31297)
        ren1.GetActiveCamera().SetFocalPoint(0, 0, 0.125)
        ren1.GetActiveCamera().SetViewAngle(30)
        ren1.GetActiveCamera().SetViewUp(0.162675, 0.952658, -0.256864)

        ren1.SetBackground(0, 0, 0)
        ren2.SetBackground(0, 0, 0)
        ren3.SetBackground(0, 0, 0)
        ren4.SetBackground(0, 0, 0)

        renWin.SetSize(500, 500)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);
        renWin.Render()

        img_file = "deciPlane.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(deciPlane, 'test')])
