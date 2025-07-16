# Fix lighting calculations for PBR interpolation

When both directional and positional lights were intermixed in the view, the polydata mapper treated
directional lights as positional and generated incorrect renderings. This issue is now fixed.

|Before|After|
|:--:|:--:|
|![](../imgs/9.5/pbr_light_before.png)|![](https://vtk.org/files/ExternalData/SHA512/01b2f3261b9dc1550b1dae1abcc32ab45c170964500115ee66efba9200ff46485d2bee6efe1e13c6972630661c29dfac5abc83de7ac88b21d566a3db215936d4)|

In the images above, there are two lights - a positional light in front of the lower left sphere and
a directional light in front of the third sphere in the second-last row.

As visible in the **Before** image, the directional light is treated as positional (see light
reflected in the neighborhood spheres) and has a conical intensity fall-off.

In the **After** image, once can see that the directional light is treated as a point light located
at an infinite distance from the scene.
