# vtkTexture wrap api

The vtkTexture API has been updated to match more closely to the OpenGL spec.

This change replaces the old ivars for Repeat and EdgeClamp with Wrap. The new SetWrap method can
accept one of { ClampToEdge, Repeat, MirroredRepeat, ClampToBorder }. In addition, we can now set a
border color to use when Wrap is set to ClampToBorder.

## Code migration

To update to the new API,

- Convert all `SetRepeat(true)`, `RepeatOn` calls to `SetWrap(vtkTexture::Repeat)`
- Convert all `SetRepeat(false)`, `RepeatOff` calls to `SetWrap(vtkTexture::ClampToEdge)`
- Remove all `SetEdgeClamp()`, `EdgeClampOn` and `EdgeClampOff` calls. This ivar was being ignored
  internally in vtkOpenGLTexture.
