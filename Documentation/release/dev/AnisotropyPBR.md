# Anisotropy in the PBR shader

The PBR shader now supports anisotropic materials. This adds two parameters
to the vtkProperty : Anisotropy (strength of the anisotropy, 1.0 means fully
anisotropic), and AnisotropyRotation (rotate the direction of anisotropy
counter-clockwise). It can also be textured with anisotropy strength in the
red channel and anisotropy rotation in the green.
