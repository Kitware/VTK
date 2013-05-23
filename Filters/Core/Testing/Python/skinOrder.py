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

import sys
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

import sys

'''
  Prevent .pyc files from being created.
  Stops the vtk source being polluted
  by .pyc files.
'''
sys.dont_write_bytecode = True

import SliceOrder

class skinOrder(vtk.test.Testing.vtkTest):

    def testSkinOrder(self):

        # Create the RenderWindow, Renderer and Interactor
        #
        ren = vtk.vtkRenderer()
        renWin = vtk.vtkRenderWindow()
        renWin.AddRenderer(ren)

        RESOLUTION = 64
        START_SLICE = 50
        END_SLICE = 60
        PIXEL_SIZE = 3.2
        centerX = RESOLUTION / 2
        centerY = RESOLUTION / 2
        centerZ = (END_SLICE - START_SLICE) / 2
        endX = RESOLUTION - 1
        endY = RESOLUTION - 1
        endZ = END_SLICE - 1
        origin = (RESOLUTION / 2.0) * PIXEL_SIZE * -1.0

        math = vtk.vtkMath()

        orders = ["ap", "pa", "si", "iss", "lr", "rl"]
        sliceOrder = SliceOrder.SliceOrder()

        reader = list()
        iso = list()
        mapper = list()
        actor = list()

        skinColors = [[0.875950, 0.598302, 0.656878],
                    [0.641134, 0.536594, 0.537889],
                    [0.804079, 0.650506, 0.558249],
                    [0.992896, 0.603716, 0.660385],
                    [0.589101, 0.513448, 0.523095],
                    [0.650247, 0.700527, 0.752458]]

        for idx, order in enumerate(orders):
            reader.append(vtk.vtkVolume16Reader())
            reader[idx].SetDataDimensions(RESOLUTION, RESOLUTION)
            reader[idx].SetFilePrefix(VTK_DATA_ROOT + '/Data/headsq/quarter')
            reader[idx].SetDataSpacing(PIXEL_SIZE, PIXEL_SIZE, 1.5)
            reader[idx].SetDataOrigin(origin, origin, 1.5)
            reader[idx].SetImageRange(START_SLICE, END_SLICE)
            if order == "ap":
                reader[idx].SetTransform(sliceOrder.ap)
            elif order == "pa":
                reader[idx].SetTransform(sliceOrder.pa)
            elif order == "si":
                reader[idx].SetTransform(sliceOrder.si)
            elif order == "iss":
                reader[idx].SetTransform(sliceOrder.iss)
            elif order == "lr":
                reader[idx].SetTransform(sliceOrder.lr)
            elif order == "rl":
                reader[idx].SetTransform(sliceOrder.rl)
            else:
                s = "No such transform exists."
                raise Exception(s)

            reader[idx].SetHeaderSize(0)
            reader[idx].SetDataMask(0x7fff)
            reader[idx].SetDataByteOrderToLittleEndian()
            reader[idx].GetExecutive().SetReleaseDataFlag(0, 1)

            iso.append(vtk.vtkContourFilter())
            iso[idx].SetInputConnection(reader[idx].GetOutputPort())
            iso[idx].SetValue(0, 550.5)
            iso[idx].ComputeScalarsOff()
            iso[idx].ReleaseDataFlagOn()

            mapper.append(vtk.vtkPolyDataMapper())
            mapper[idx].SetInputConnection(iso[idx].GetOutputPort())
            mapper[idx].ImmediateModeRenderingOn()

            actor.append(vtk.vtkActor())
            actor[idx].SetMapper(mapper[idx])
#            r = math.Random(.5, 1)
#            g = math.Random(.5, 1)
#            b = math.Random(.5, 1)
#            print r, g, b
            actor[idx].GetProperty().SetDiffuseColor(
#                        math.Random(.5, 1), math.Random(.5, 1), math.Random(.5, 1))
#                        r, g, b)
                        skinColors[idx])
            ren.AddActor(actor[idx])


        renWin.SetSize(300, 300)
        ren.ResetCamera()
        ren.GetActiveCamera().Azimuth(210)
        ren.GetActiveCamera().Elevation(30)
        ren.GetActiveCamera().Dolly(1.2)
        ren.ResetCameraClippingRange()

        ren.SetBackground(.8, .8, .8)

        # render and interact with data

        iRen = vtk.vtkRenderWindowInteractor()
        iRen.SetRenderWindow(renWin);

        renWin.Render()

        img_file = "skinOrder.png"
        vtk.test.Testing.compareImage(iRen.GetRenderWindow(), vtk.test.Testing.getAbsImagePath(img_file), threshold=25)
        vtk.test.Testing.interact()

if __name__ == "__main__":
     vtk.test.Testing.main([(skinOrder, 'test')])
