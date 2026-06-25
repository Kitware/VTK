# Camera Manipulators Enhanced

VTK now provides a simpler way to bind different kinds of camera movement to a combination of mouse button and modifier keys. The concept of interaction style manipulators is not new and existed in ParaView for a long time. VTK now provides that functionality, so you can easily setup key bindings for the view using the `vtkInteractorStyleManipulator` class. This class lets you add manipulators that are bound to specific mouse and modifier key (`Alt`, `Ctrl`, `Shift`) combinations. With several built-in manipulators available, you can bind each to different button and modifier combination without writing custom interactor styles!

For example, here is a sample code snippet demonstrating how to add a manipulators:

```py
from vtkmodules.vtkInteractionStyle import (vtkCameraManipulator,
                                            vtkInteractorStyleManipulator,
                                            vtkJoystickFlyIn,
                                            vtkJoystickFlyOut,
                                            vtkTrackballRotate,
                                            vtkTrackballPan,
                                            vtkTrackballZoom,
                                            vtkTrackballRoll,
                                            vtkTableTopRotate,
                                            vtkTrackballZoomToMouse,
                                            vtkTrackballEnvironmentRotate,
                                            vtkTrackballMultiRotate)
from vtkmodules.vtkRenderingCore import vtkRenderWindowInteractor
# An example that assigns different mouse and modifier key combinations to particular manipulators.
# Each row corresponds to a modifier key (no modifier, Ctrl, Shift, Alt) and each column corresponds to a mouse button (Left, Middle, Right).
manipulator_classes = [
    # no modifier¡
    # Left, Middle, Right
    [vtkTrackballRotate, vtkTrackballPan, vtkTrackballZoom],
    # Ctrl + Left, Ctrl + Middle, Ctrl + Right
    [vtkTrackballEnvironmentRotate, vtkTrackballRoll, vtkTrackballZoomToMouse],
    # Shift + Left, Shift + Middle, Shift + Right
    [vtkTableTopRotate, vtkTrackballRoll, vtkTrackballMultiRotate],
    # Alt + Left, Alt + Middle, Alt + Right
    [vtkJoystickFlyIn, vtkJoystickFlyOut, vtkTrackballMultiRotate],
]
buttons = [vtkCameraManipulator.MouseButtonType.Left, vtkCameraManipulator.MouseButtonType.Middle, vtkCameraManipulator.MouseButtonType.Right]
ctrl_modifier = [False, False, True]
shift_modifier = [False, True, False]

style_manip = vtk.vtkInteractorStyleManipulator()
for row_id, row in enumerate(manipulator_classes):
    for column_id, manipulator_class in enumerate(row):
        style_manip.AddManipulator(
            manipulator_class(mouse_button=buttons[column_id],
                              control=ctrl_modifier[row_id],
                              shift=shift_modifier[row_id],
                            )
                        )
interactor = vtk.vtkRenderWindowInteractor(interactor_style=style_manip)
interactor.SetInteractorStyle(style_manip)
```
