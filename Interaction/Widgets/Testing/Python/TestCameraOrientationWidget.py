#!/usr/bin/env python
# -*- coding: utf-8 -*-

'''
=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedColorsIntegration.py

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http:#www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================
'''

import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
import os
VTK_DATA_ROOT = vtkGetDataRoot()

# This example demonstrates how to use the vtkCameraOrientationWidget to control
# a renderer's camera orientation.

# -Z -> -X -> -Z
FromMinusZToMinusX = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 294 265 0 0 0 c\n\
          LeftButtonPressEvent 294 265 0 0 0 c\n\
          LeftButtonReleaseEvent 294 265 0 0 0 c\n"
FromMinusXToMinusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 289 0 0 0 c\n\
          LeftButtonPressEvent 267 289 0 0 0 c\n\
          LeftButtonReleaseEvent 267 289 0 0 0 c\n"
###
# -Z -> -Y -> -Z
FromMinusZToMinusY = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 268 288 0 0 0 c\n\
          LeftButtonPressEvent 268 288 0 0 0 c\n\
          LeftButtonReleaseEvent 268 288 0 0 0 c\n"
FromMinusYToMinusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 289 0 0 0 c\n\
          LeftButtonPressEvent 267 289 0 0 0 c\n\
          LeftButtonReleaseEvent 267 289 0 0 0 c\n"
###
# -Z -> +Z -> -Z
FromMinusZToPlusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 265 0 0 0 c\n\
          LeftButtonPressEvent 267 265 0 0 0 c\n\
          LeftButtonReleaseEvent 267 265 0 0 0 c\n"
FromPlusZToMinusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 265 0 0 0 c\n\
          LeftButtonPressEvent 267 265 0 0 0 c\n\
          LeftButtonReleaseEvent 267 265 0 0 0 c\n"
###
# +Z -> +X -> +Z
FromPlusZToPlusX = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 289 265 0 0 0 c\n\
          LeftButtonPressEvent 289 265 0 0 0 c\n\
          LeftButtonReleaseEvent 289 265 0 0 0 c\n"
FromPlusXToPlusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 245 0 0 0 c\n\
          LeftButtonPressEvent 267 245 0 0 0 c\n\
          LeftButtonReleaseEvent 267 245 0 0 0 c\n"
###
# +Z -> +Y -> +Z
FromPlusZToPlusY = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 245 0 0 0 c\n\
          LeftButtonPressEvent 267 245 0 0 0 c\n\
          LeftButtonReleaseEvent 267 245 0 0 0 c\n"
FromPlusYToPlusZ = "# StreamVersion 1.1\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 267 245 0 0 0 c\n\
          LeftButtonPressEvent 267 245 0 0 0 c\n\
          LeftButtonReleaseEvent 267 245 0 0 0 c\n"

###
# -Z -> arbitrary
FromMinusZToArbitrary = "# StreamVersion 1.1\n\
          LeaveEvent 300 150 0 0 0 c\n\
          EnterEvent 150 150 0 0 0 c\n\
          MouseMoveEvent 268 288 0 0 0 c\n\
          LeftButtonPressEvent 268 288 0 0 0 c\n\
          MouseMoveEvent 266 287 0 0 0 c\n\
          MouseMoveEvent 260 260 0 0 0 c\n\
          MouseMoveEvent 250 255 0 0 0 c\n\
          MouseMoveEvent 246 250 0 0 0 c\n\
          LeftButtonReleaseEvent 246 250 0 0 0 c\n"


class TestCameraOrientationWidget(vtk.test.Testing.vtkTest):

    def spin(self, instructions, widgetBack, widgetUp, camPos, focalPoint, viewUp):
        self.recorder.SetInputString(instructions)
        self.recorder.Play()

        rep = self.camOrientManipulator.GetRepresentation()
        cam = self.renderer.GetActiveCamera()
        for i in range(3):
            widgetBack[i] = rep.GetBack()[i]
            widgetUp[i] = rep.GetUp()[i]

            camPos[i] = cam.GetPosition()[i]
            focalPoint[i] = cam.GetFocalPoint()[i]
            viewUp[i] = cam.GetViewUp()[i]

    def testCameraOrientationWidget(self):
        self.camOrientManipulator = vtk.vtkCameraOrientationWidget()
        self.renderer = vtk.vtkRenderer()
        self.renWin = vtk.vtkRenderWindow()
        self.interactor = vtk.vtkRenderWindowInteractor()
        self.recorder = vtk.vtkInteractorEventRecorder()

        reader = vtk.vtkXMLPolyDataReader()
        reader.SetFileName(os.path.join(VTK_DATA_ROOT, "Data/cow.vtp"))

        mapper = vtk.vtkPolyDataMapper()
        mapper.SetInputConnection(reader.GetOutputPort())

        actor = vtk.vtkActor()
        actor.SetMapper(mapper)

        self.renderer.AddActor(actor)
        self.renderer.SetBackground(0.32, 0.32, 0.32)
        self.renWin.AddRenderer(self.renderer)
        self.renWin.SetSize(300, 300)

        self.interactor.SetRenderWindow(self.renWin)
        self.interactor.Initialize()

        self.camOrientManipulator.SetParentRenderer(self.renderer)
        self.camOrientManipulator.On()

        self.renWin.Render()

        self.recorder.SetInteractor(self.interactor)
        self.recorder.ReadFromInputStringOn()

        instructions = [FromMinusZToMinusX, FromMinusXToMinusZ,
                        FromMinusZToMinusY, FromMinusYToMinusZ,
                        FromMinusZToPlusZ,  FromPlusZToMinusZ,
                        FromMinusZToPlusZ,  FromPlusZToPlusX,
                        FromPlusXToPlusZ,   FromPlusZToPlusY,
                        FromPlusYToPlusZ,   FromPlusZToMinusZ,
                        FromMinusZToArbitrary]

        requiredWidgetBack = [
            [-1, 0, 0], [0, 0, -1], [0, -1, 0], [0, 0, -1],
            [0, 0, 1],  [0, 0, -1], [0, 0, 1],  [1, 0, 0],
            [0, 0, 1],  [0, 1, 0],  [0, 0, 1],  [0, 0, -1],
            [-0.44760227022559895, -0.7930977224238861, -0.4130958852069752]
        ]
        requiredWidgetUp = [[0, 0, 1], [0, 1, 0], [0, 0, 1],
                            [0, 1, 0], [0, 1, 0], [0, 1, 0],
                            [0, 1, 0], [0, 0, 1], [0, 1, 0],
                            [0, 0, 1], [0, 1, 0], [0, 1, 0],
                            [-0.24830554277755906,
                                0.5540205026572317, -0.7946103699684332]
                            ]
        requiredPos = [
            [25.3322, -0.438658, 0],        [0.776126, -0.438658, 24.556],
            [0.776126, 24.1174, 0],         [0.776126, -0.438658, 24.556],
            [0.776126, -0.438658, -24.556], [0.776126, -0.438658, 24.556],
            [0.776126, -0.438658, -24.556], [-23.7799, -0.438658, 0],
            [0.776126, -0.438658, -24.556], [0.776126, -24.9947, 0],
            [0.776126, -0.438658, -24.556], [0.776126, -0.438658, 24.556],
            [11.767466031971992, 19.036682097465544, 10.143999445975485]
        ]

        requiredFp = [
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.776126, -0.438658, 0], [0.776126, -0.438658, 0],
            [0.7761263847351074, -0.4386579990386963, 0.0]
        ]
        requiredViewUp = [[0, 0, 1], [0, 1, 0], [0, 0, 1],
                          [0, 1, 0], [0, 1, 0], [0, 1, 0],
                          [0, 1, 0], [0, 0, 1], [0, 1, 0],
                          [0, 0, 1], [0, 1, 0], [0, 1, 0],
                          [-0.24830554277755904,
                              0.5540205026572318, -0.7946103699684332]
                          ]

        widgetBack = [0, 0, 0]
        widgetUp = [0, 0, 0]
        camPos = [0, 0, 0]
        focalPoint = [0, 0, 0]
        viewUp = [0, 0, 0]
        for i in range(13):
            self.spin(
                instructions[i], widgetBack, widgetUp, camPos, focalPoint, viewUp)
            # verify widget, camera orientations
            for j in range(3):
                self.assertAlmostEqual(widgetBack[j], requiredWidgetBack[i][j])
                self.assertAlmostEqual(widgetUp[j], requiredWidgetUp[i][j])
                self.assertAlmostEqual(camPos[j], requiredPos[i][j], places=4)
                self.assertAlmostEqual(
                    focalPoint[j], requiredFp[i][j], places=4)
                self.assertAlmostEqual(
                    viewUp[j], requiredViewUp[i][j], places=4)

        # Remove the observers so we can go interactive. Without this the "-I"
        # testing option fails.
        self.recorder.Off()
        vtk.test.Testing.compareImage(self.renWin, vtk.test.Testing.getAbsImagePath(
            "TestCameraOrientationWidget.png"))
        vtk.test.Testing.interact()


if __name__ == "__main__":
    vtk.test.Testing.main([(TestCameraOrientationWidget, 'test')])
