## Support point size in vtkGlyph3DMapper OpenGL backend in WASM

The `vtkOpenGLGlyph3DHelper` class did not support point size on the WASM platform, when using GLES3.
This caused glyphs rendered with point representation to appear very small and hard to see irrespective
of the point size set on the actor property. This bug is now fixed by correctly propagating the point size
to the shaders.
