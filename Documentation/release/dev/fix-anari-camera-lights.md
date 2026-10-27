## ANARI now lights a scene the way the OpenGL backend does

VTK sets a renderer's lights up as part of drawing a frame: it creates the
default headlight when the scene has no light of its own, and it moves the
lights that are placed relative to the camera. `vtkOpenGLRenderer` only does
that work when it draws the scene itself, and nothing did it on an ANARI
render -- `vtkAnariPass` takes the place of that code path, and
`vtkAnariRenderWindow` replaces the render entirely.

A scene that relied on the automatically created headlight therefore reached
ANARI with no light at all, and a headlight or camera light that an application
had added stayed wherever it happened to be when the first frame was drawn, so
the image went dark as soon as the camera was rotated away from its starting
orientation. `vtkAnariSceneGraph` now sets the lights up before it builds the
scene, which covers both entry points, and `vtkAnariLightNode` tracks the camera
as well as the light, since a camera light is placed with a transform matrix
that leaves the light's own modification time untouched. A replaced light also
reaches the world again: the world used to keep holding light handles that had
already been released, which made each successive render of the same scene
darker than the last.

A matte sphere lit by a headlight now holds a mean brightness of 0.774 through
ANARI with VisRTX at every camera azimuth, matching the OpenGL backend; the same
sweep previously read zero. A fixed scene light follows the same cosine falloff
through both backends.
