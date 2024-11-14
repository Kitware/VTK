#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkIOXML import vtkXMLPolyDataReader
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkInteractorEventRecorder,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
import os
VTK_DATA_ROOT = vtkGetDataRoot()

# This example demonstrates how to use the vtkCameraOrientationWidget to control
# a renderer's camera orientation.

# -Z -> -X -> -Z
FromMinusZToMinusX = """# StreamVersion 1.2
ExposeEvent 0 299 0 0 0 0 0
EnterEvent 298 224 0 0 0 0 0
MouseMoveEvent 283 230 0 0 0 0 0
LeftButtonPressEvent 283 230 0 0 0 0 0
LeftButtonReleaseEvent 283 230 0 0 0 0 0
MouseMoveEvent 282 230 0 0 0 0 0
"""
FromMinusXToMinusZ = """# StreamVersion 1.2
MouseMoveEvent 230 275 0 0 0 0 0
LeftButtonPressEvent 230 275 0 0 0 0 0
LeftButtonReleaseEvent 230 275 0 0 0 0 0
MouseMoveEvent 282 275 0 0 0 0 0
"""
###
# -Z -> -Y -> -Z
FromMinusZToMinusY = """# StreamVersion 1.2
MouseMoveEvent 232 276 0 0 0 0 0
LeftButtonPressEvent 232 276 0 0 0 0 0
LeftButtonReleaseEvent 232 276 0 0 0 0 0
MouseMoveEvent 232 276 0 0 0 0 0
"""
FromMinusYToMinusZ = """# StreamVersion 1.2
MouseMoveEvent 231 274 0 0 0 0 0
LeftButtonPressEvent 231 274 0 0 0 0 0
LeftButtonReleaseEvent 231 274 0 0 0 0 0
MouseMoveEvent 231 274 0 0 0 0 0
"""
###
# -Z -> +Z
FromMinusZToPlusZ = """# StreamVersion 1.2
MouseMoveEvent 226 229 0 0 0 0 0
LeftButtonPressEvent 226 229 0 0 0 0 0
LeftButtonReleaseEvent 226 229 0 0 0 0 0
MouseMoveEvent 226 229 0 0 0 0 0
"""
FromPlusZToMinusZ = """# StreamVersion 1.2
MouseMoveEvent 227 229 0 0 0 0 0
LeftButtonPressEvent 227 229 0 0 0 0 0
LeftButtonReleaseEvent 227 229 0 0 0 0 0
MouseMoveEvent 227 229 0 0 0 0 0
"""
###
# +Z -> +X -> +Z
FromPlusZToPlusX = """# StreamVersion 1.2
MouseMoveEvent 269 227 0 0 0 0 0
LeftButtonPressEvent 269 227 0 0 0 0 0
LeftButtonReleaseEvent 269 227 0 0 0 0 0
MouseMoveEvent 269 227 0 0 0 0 0
"""
FromPlusXToPlusZ = """# StreamVersion 1.2
MouseMoveEvent 235 186 0 0 0 0 0
LeftButtonPressEvent 235 186 0 0 0 0 0
LeftButtonReleaseEvent 235 186 0 0 0 0 0
MouseMoveEvent 235 186 0 0 0 0 0
"""
###
# +Z -> +Y -> +Z
FromPlusZToPlusY = """# StreamVersion 1.2
MouseMoveEvent 236 181 0 0 0 0 0
LeftButtonPressEvent 236 181 0 0 0 0 0
LeftButtonReleaseEvent 236 181 0 0 0 0 0
MouseMoveEvent 236 181 0 0 0 0 0
"""
FromPlusYToPlusZ = """# StreamVersion 1.2
MouseMoveEvent 229 180 0 0 0 0 0
LeftButtonPressEvent 229 180 0 0 0 0 0
LeftButtonReleaseEvent 229 180 0 0 0 0 0
MouseMoveEvent 229 180 0 0 0 0 0
"""

###
# -Z -> arbitrary
FromMinusZToArbitrary = """# StreamVersion 1.2
MouseMoveEvent 232 277 0 0 0 0 0
LeftButtonPressEvent 232 277 0 0 0 0 0
MouseMoveEvent 231 277 0 0 0 0 0
MouseMoveEvent 212 229 0 0 0 0 0
MouseMoveEvent 210 228 0 0 0 0 0
MouseMoveEvent 208 226 0 0 0 0 0
LeftButtonReleaseEvent 178 187 0 0 0 0 0
MouseMoveEvent 178 187 0 0 0 0 0
"""

class TestCameraOrientationWidget(vtkmodules.test.Testing.vtkTest):

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
        self.camOrientManipulator = vtkCameraOrientationWidget()
        self.renderer = vtkRenderer()
        self.renWin = vtkRenderWindow()
        self.interactor = vtkRenderWindowInteractor()
        self.recorder = vtkInteractorEventRecorder()

        reader = vtkXMLPolyDataReader()
        reader.SetFileName(os.path.join(VTK_DATA_ROOT, "Data/cow.vtp"))

        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(reader.GetOutputPort())

        actor = vtkActor()
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
            [-0.2534933352606766, -0.6182851620138625, -0.7439520061213022]
        ]
        requiredWidgetUp = [[0, 0, 1], [0, 1, 0], [0, 0, 1],
                            [0, 1, 0], [0, 1, 0], [0, 1, 0],
                            [0, 1, 0], [0, 0, 1], [0, 1, 0],
                            [0, 0, 1], [0, 1, 0], [0, 1, 0],
                            [-0.16187898668927914,
                                0.7853407471227284, -0.597524145601805]
                            ]
        requiredPos = [
            [25.3322, -0.438658, 0],        [0.776126, -0.438658, 24.556],
            [0.776126, 24.1174, 0],         [0.776126, -0.438658, 24.556],
            [0.776126, -0.438658, -24.556], [0.776126, -0.438658, 24.556],
            [0.776126, -0.438658, -24.556], [-23.7799, -0.438658, 0],
            [0.776126, -0.438658, -24.556], [0.776126, -24.9947, 0],
            [0.776126, -0.438658, -24.556], [0.776126, -0.438658, 24.556],
            [7.000919089107853, 14.743977717076099, 18.268515877725843]
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
                          [-0.16187898668927914,
                              0.7853407471227285, -0.5975241456018049]
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
        vtkmodules.test.Testing.compareImage(self.renWin, vtkmodules.test.Testing.getAbsImagePath(
            "TestCameraOrientationWidget.png"))
        vtkmodules.test.Testing.interact()


if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestCameraOrientationWidget, 'test')])
