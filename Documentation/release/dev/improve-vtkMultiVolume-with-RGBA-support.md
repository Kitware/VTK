# Add support for RGBA inputs in vtkMultiVolume

vtkMultiVolume now supports RGBA volume inputs in a similar way to the existing single-input
volume rendering. When turning off the IndependentComponent flag of the vtkVolumeProperty while
providing 4-components inputs to the mapper, the first three components are interpreted as RGB
colors and are not mapped through the color tranfer function. The fourth component is mapped
through the scalar opacity function.
