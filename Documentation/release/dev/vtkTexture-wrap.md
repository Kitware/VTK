# vtkTexture wrap api

The vtkTexture API has been updated to match more closely to the OpenGL spec.

This change replaces the old ivars for Repeat and EdgeClamp with Wrap. The new SetWrap method can
accept one of { ClampToEdge, Repeat, MirroredRepeat, ClampToBorder }. In addition, we can now set a
border color to use when Wrap is set to ClampToBorder.
