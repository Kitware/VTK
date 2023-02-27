## Add 3D Cursor Widget for stereo displays

You can now track the mouse or any other pointing device in the scene using the new
vtk3DCursorWidget and vtk3DCursorRepresentation classes.
The 3D cursor follows the mouse and is placed on the surface of the actors in the scene if
the mouse hovers them, or on the focal plane of the camera otherwise.
This widget is intended to be used when using VTK with a stereo display, in place of the 2D mouse cursor.

The described behavior does not currently work with volumes.
