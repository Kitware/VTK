## Add new VR movement style

Add the ability to choose between two movement styles for VR interactions in `vtkVRInteractorStyle`.
The first one is named "Flying" and was the default style until now. You can move in the scene according to the position of the controller
(the left one by default) by pushing the joystick up or down.
The second one is named "Grounded". It allows you to move on the `XY` (ground) plan using the four directions
of the joystick (the left one by default), according to the view direction of the headset. You can also modify the elevation of the scene
by pushing the right joystick (by default) up or down.
