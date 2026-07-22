#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""Test that vtkCameraOrientationWidget and vtkInteractorStyleManipulator
respect the center of rotation.

Automated assertions:
  - Clicking the +X handle of the gizmo snaps the camera with a rigid 90
    degree rotation about the view-up axis through the center of rotation.
  - A 180 degree left-drag with vtkTrackballRotate mirrors the camera
    position and focal point about the center of rotation in the x-z plane.

Interaction (with -I):
  - Left drag rotates the camera with vtkTrackballRotate.
  - Press 'c', then left click: the picked world position becomes the
    center of rotation (marked with a red sphere).
  - Click the camera orientation widget axes to snap the camera about the
    center of rotation.
"""

import os

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkInteractionStyle import (
    vtkCameraManipulator,
    vtkInteractorStyleManipulator,
    vtkTrackballRotate,
    vtkTrackballPan,
)
from vtkmodules.vtkIOXML import vtkXMLPolyDataReader
from vtkmodules.vtkInteractionWidgets import vtkCameraOrientationWidget
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCellPicker,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
from vtkmodules.util.misc import vtkGetDataRoot
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing

VTK_DATA_ROOT = vtkGetDataRoot()


class TestCameraOrientationWidgetCenterOfRotation(vtkmodules.test.Testing.vtkTest):
    def assertTupleAlmostEqual(self, actual, expected, places=4):
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a, e, places=places)

    def testCameraOrientationWidgetCenterOfRotation(self):
        renderer = vtkRenderer()
        renderer.SetBackground(0.32, 0.32, 0.32)

        # 300x300 so the (283, 230) gizmo handle position from
        # TestCameraOrientationWidget.py stays valid.
        renWin = vtkRenderWindow()
        renWin.SetSize(300, 300)
        renWin.AddRenderer(renderer)

        interactor = vtkRenderWindowInteractor()
        interactor.SetRenderWindow(renWin)

        # Manipulator style with rotation on the left button.
        style = vtkInteractorStyleManipulator()
        rotate = vtkTrackballRotate()
        rotate.SetMouseButton(vtkCameraManipulator.MouseButtonType.Left)
        style.AddManipulator(rotate)
        pan = vtkTrackballPan()
        pan.SetMouseButton(vtkCameraManipulator.MouseButtonType.Middle)
        style.AddManipulator(pan)
        interactor.SetInteractorStyle(style)

        reader = vtkXMLPolyDataReader()
        reader.SetFileName(os.path.join(VTK_DATA_ROOT, "Data/cow.vtp"))

        mapper = vtkPolyDataMapper()
        mapper.SetInputConnection(reader.GetOutputPort())
        actor = vtkActor()
        actor.SetMapper(mapper)
        renderer.AddActor(actor)

        # Red sphere marking the current center of rotation. Not pickable so
        # it never captures its own placement click.
        sphere = vtkSphereSource()
        sphere.SetRadius(0.05)
        sphere.SetThetaResolution(32)
        sphere.SetPhiResolution(32)
        markerMapper = vtkPolyDataMapper()
        markerMapper.SetInputConnection(sphere.GetOutputPort())
        marker = vtkActor()
        marker.SetMapper(markerMapper)
        marker.GetProperty().SetColor(1.0, 0.0, 0.0)
        marker.PickableOff()
        renderer.AddActor(marker)

        renderer.ResetCamera()

        picker = vtkCellPicker()
        picker.SetTolerance(0.005)

        armed = [False]
        pressTag = [None]

        def onKeyPress(obj, event):
            if obj.GetKeySym() in ("c", "C"):
                armed[0] = True

        def onLeftButtonPress(obj, event):
            if not armed[0]:
                return
            armed[0] = False
            x, y = obj.GetEventPosition()
            if picker.Pick(x, y, 0, renderer):
                center = picker.GetPickPosition()
                style.SetCenterOfRotation(center)
                marker.SetPosition(center)
                print("Center of rotation:", center)
                renWin.Render()
            # Consume the event so the rotate manipulator does not start.
            obj.GetCommand(pressTag[0]).AbortFlagOn()

        interactor.AddObserver("KeyPressEvent", onKeyPress, 1.0)
        pressTag[0] = interactor.AddObserver(
            "LeftButtonPressEvent", onLeftButtonPress, 1.0
        )

        interactor.Initialize()

        camOrientWidget = vtkCameraOrientationWidget(should_reset_camera=False)
        camOrientWidget.UseCenterOfRotationOn()
        # Snap immediately instead of animating so assertions see the final
        # camera state right after the button release.
        camOrientWidget.AnimateOff()
        camOrientWidget.SetParentRenderer(renderer)
        camOrientWidget.On()

        renWin.Render()

        cam = renderer.GetActiveCamera()

        # ------------------------------------------------------------------
        # Part 1: widget snap about the center of rotation.
        # The camera starts on +Z looking down -Z with view up +Y. Clicking
        # the gizmo handle at (283, 230) snaps the camera to the +X side with
        # the handle's canonical view up +Z. The rotation mapping the frame
        # (right +X, up +Y, dir -Z) onto (right +Y, up +Z, dir -X) is the
        # cyclic axis permutation rel (x, y, z) -> (z, x, y), applied about
        # the center.
        center = (0.3, -0.15, 0.1)
        style.SetCenterOfRotation(center)
        marker.SetPosition(center)

        posBefore = tuple(cam.GetPosition())
        fpBefore = tuple(cam.GetFocalPoint())

        def snapRotatedAboutCenter(p, c):
            rel = (p[0] - c[0], p[1] - c[1], p[2] - c[2])
            return (c[0] + rel[2], c[1] + rel[0], c[2] + rel[1])

        # hover to make the handle hot, then click it.
        interactor.SetEventInformation(283, 230, 0, 0)
        interactor.InvokeEvent("MouseMoveEvent")
        interactor.InvokeEvent("LeftButtonPressEvent")
        interactor.InvokeEvent("LeftButtonReleaseEvent")

        self.assertTupleAlmostEqual(
            cam.GetPosition(), snapRotatedAboutCenter(posBefore, center))
        self.assertTupleAlmostEqual(
            cam.GetFocalPoint(), snapRotatedAboutCenter(fpBefore, center))
        self.assertTupleAlmostEqual(cam.GetViewUp(), (0.0, 0.0, 1.0))

        # ------------------------------------------------------------------
        # Part 2: trackball rotate about the center of rotation.
        # A pure horizontal drag of 150 px in a 300 px window azimuths the
        # camera by 360 * 150 / 300 = 180 degrees about the view-up axis
        # through the center. The view up is +Z after the snap above, so the
        # camera position and focal point are mirrored about the center in
        # the x-y plane.
        center = (0.25, 0.1, -0.2)
        style.SetCenterOfRotation(center)
        marker.SetPosition(center)

        posBefore = tuple(cam.GetPosition())
        fpBefore = tuple(cam.GetFocalPoint())

        # Move off the gizmo so the widget leaves its hot state; otherwise it
        # grabs the button press below instead of the interactor style.
        interactor.SetEventInformation(150, 150, 0, 0)
        interactor.InvokeEvent("MouseMoveEvent")

        interactor.SetEventInformation(225, 150, 0, 0)
        interactor.InvokeEvent("LeftButtonPressEvent")
        for x in range(215, 74, -10):
            interactor.SetEventInformation(x, 150, 0, 0)
            interactor.InvokeEvent("MouseMoveEvent")
        interactor.InvokeEvent("LeftButtonReleaseEvent")

        self.assertTupleAlmostEqual(
            cam.GetPosition(),
            (2.0 * center[0] - posBefore[0], 2.0 * center[1] - posBefore[1],
             posBefore[2]))
        self.assertTupleAlmostEqual(
            cam.GetFocalPoint(),
            (2.0 * center[0] - fpBefore[0], 2.0 * center[1] - fpBefore[1],
             fpBefore[2]))
        self.assertTupleAlmostEqual(cam.GetViewUp(), (0.0, 0.0, 1.0))

        # ------------------------------------------------------------------
        # Reset to a clean view for interactive use.
        renderer.ResetCamera()
        initialCenter = cam.GetFocalPoint()
        style.SetCenterOfRotation(initialCenter)
        marker.SetPosition(initialCenter)
        renWin.Render()

        if vtkmodules.test.Testing.isInteractive():
            camOrientWidget.AnimateOn()
            interactor.Start()


if __name__ == "__main__":
    vtkmodules.test.Testing.main(
        [(TestCameraOrientationWidgetCenterOfRotation, "test")]
    )
