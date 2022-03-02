## Add support for specifying coordinate systems for Prop3D

**Goal:** Make it easy to add Actors in other coordinate frames than world

Add the notion of what coordinate system a Prop3D is in such as

- WORLD (the default)
- PHYSICAL (in VR, the physical room space in meters)
- DEVICE (relative to a device such as a Head Mounted Display)

When PHYSCIAL or DEVICE is specified you must provide the
renderer that the coordinate system is related to through
SetCoordinateSystemRenderer().

When the coordinate system is set to DEVICE you should
SetCoordinateSystemDevice to indicate what device's space
the Prop3D is in reference to.

See the OpenVR TestDragon test for an example of how to use.

Note that GetBounds and ComputeMatrix have been modified for this
so analytic picking and similar operations should work.

To support this some additional pose/transformation methods were
added to vtkMatrix4x4.

Note: that when using PHYSCIAL or DEVICE coordinate systems you
probably want to mark those props as UseBoundsOff() so that their
bounds are not part of the bounds used when resetting the camera.
