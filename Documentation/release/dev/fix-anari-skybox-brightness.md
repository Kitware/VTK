## Fix skybox brightness with ANARI

`vtkAnariPass` now draws skyboxes with their OpenGL shader, preserving projection,
gamma correction, and foreground depth. Skyboxes no longer become illuminated
geometry in the ANARI scene.

Environment textures used as HDRI lights now normalize unsigned-char colors and
convert sRGB colors to linear radiance. Linear floating-point HDR values retain
their range. Changes to environment textures or the image-based lighting setting
also update the ANARI light.

![Studio HDRI lighting](anari-skybox-studio.png)

Polished metals and glazed ceramics illuminated by a studio HDRI.

![Moonlit scientific visualization](anari-skybox-moonlit.png)

Lorenz convection trajectories colored by phase-space speed above moonlit water.

![Moonlit medical volume rendering](anari-skybox-medical.png)

Direct volume rendering of the Slicer CTBrain sample illuminated by a moonlit HDRI.
