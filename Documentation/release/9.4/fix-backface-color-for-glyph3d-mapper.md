# Fix backface color in glyph 3D mapper

Fixes a bug in the OpenGL implementation of `vtkGlyph3DMapper` that did not propagate backface colors
from the actor to the OpenGL shader program. You can now set the back face color of an actor connected
to a `vtkGlyph3DMapper` and see the interior faces of the glyph mesh with a different color than the
front faces.
