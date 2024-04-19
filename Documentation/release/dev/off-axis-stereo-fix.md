## Off-axis stereo image separation issue fixed

Fix how the final transformation, T, described in "Generalized Perspective
Projection" is applied when doing off-axis projection.  This transform
encapsulates the viewpoint offset, and should be computed using the same eye
positions used to compute the rest of the projection matrix, but was
inadvertently computed using the single EyePosition.

The result of the bug was that stereo images of an object were always a fixed
distance apart from each other, regardless of the objects location w.r.t. the
screen, and regardless of the the eye position w.r.t. the object and screen.

This change moves computation of the left/right eye into a new method
GetStereoEyePosition() and uses that from both the projection and view transform
computations.

Also adds a test to render three disks at different locations with respect to
the screen to ensure the stereo image pairs behave correctly.
