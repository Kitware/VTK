# Improve GPU Volumetric Rendering

## Improve stability

VTK now detects more accurately unstable configurations (eg multi volume with incompatible lighting)
and disable the rendering in those cases.

## Improve performance

The shader composer now factorizes more data used during the raymarching loop.
You can also set more than 6 lights now. The system passing lights as uniforms
has also been improved.

## Add shadow rays

When the shading of a volume is enabled, you can now render volumetric shadows. These shadows are cast by the volume on itself,
not by surfaces. There is now an option in vtkVolumeMapper to enable it. This option controls the volumetric scattering blending
between the gradient-based shading approach, and the volumetric scattering model with shadow rays.
This option is considered only if the shading is also enabled.
A global illumination reach parameter controls the scope of secondary rays. A value of 0 means only local shadows and 1 means
that shadows will be computed through the whole volume. The higher this parameter is, the slower the rendering will be.

## Anisotropy

You can now specify a volume anisotropy in the volume's property, which will be used in the volumetric scattering model.
OSPRay will also use this value instead of its own, which is being deprecated.
`vtkVolumeProperty::SetScatteringAnisotropy` and `vtkVolumeProperty::GetScatteringAnisotropy` should now be used instead
of the deprecated `vtkOSPRayRendererNode::VOLUME_ANISOTROPY`, `vtkOSPRayRendererNode::GetVolumeAnisotropy`
and `vtkOSPRayRendererNode::SetVolumeAnistropy`.
