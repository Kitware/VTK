#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkInteractorEventRecorder,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.vtkFiltersSources import vtkSphereSource
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

INITIAL_CAMERA_DISTANCE = 100.0

FromMinusZToMinusX = """# StreamVersion 1.2
ExposeEvent 0 299 0 0 0 0 0
EnterEvent 298 224 0 0 0 0 0
MouseMoveEvent 283 230 0 0 0 0 0
LeftButtonPressEvent 283 230 0 0 0 0 0
LeftButtonReleaseEvent 283 230 0 0 0 0 0
MouseMoveEvent 282 230 0 0 0 0 0
"""

def cameraDistance(camera):
    pos = camera.GetPosition()
    fp = camera.GetFocalPoint()
    return ((pos[0] - fp[0]) ** 2 + (pos[1] - fp[1]) ** 2 + (pos[2] - fp[2]) ** 2) ** 0.5


class TestCameraOrientationWidgetNoReset(vtkmodules.test.Testing.vtkTest):
    def testShouldNotResetCamera(self):
        renderer = vtkRenderer()
        renWin = vtkRenderWindow()
        renWin.AddRenderer(renderer)
        renWin.SetSize(300, 300)

        interactor = vtkRenderWindowInteractor()
        interactor.SetRenderWindow(renWin)

        source = vtkSphereSource()
        source.SetRadius(10.0)
        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(source.GetOutputPort())
        actor = vtkActor()
        actor.SetMapper(mapper)
        renderer.AddActor(actor)

        widget = vtkCameraOrientationWidget(parent_renderer=renderer,
                                            should_reset_camera=False,
                                            animate=False)

        interactor.Initialize()
        widget.On()

        camera = renderer.GetActiveCamera()
        # Offset X by 10 to simulate a pan event.
        camera.SetFocalPoint(10.0, 0.0, 0.0)
        camera.SetPosition(10.0, 0.0, INITIAL_CAMERA_DISTANCE)
        camera.SetViewUp(0.0, 1.0, 0.0)
        renderer.ResetCameraClippingRange()

        renWin.Render()
        distanceBefore = cameraDistance(camera)
        # sanity check that the camera is at the expected distance before the interaction.
        self.assertAlmostEqual(distanceBefore, INITIAL_CAMERA_DISTANCE, places=6)

        recorder = vtkInteractorEventRecorder()
        recorder.SetInteractor(interactor)
        recorder.ReadFromInputStringOn()
        recorder.SetInputString(FromMinusZToMinusX)
        recorder.Play()

        renWin.Render()
        distanceAfter = cameraDistance(camera)
        # If camera was reset, distanceAfter would be 65.807 != 100
        self.assertAlmostEqual(distanceAfter, distanceBefore, places=6)


if __name__ == "__main__":
    vtkmodules.test.Testing.main([(TestCameraOrientationWidgetNoReset, 'test')])
