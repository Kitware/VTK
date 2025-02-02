# Fix vertex visibility in OpenGL mappers

You can now see vertices of a mesh by calling `actor->SetVertexVisibility(true)`.
Previously, a bug in the VTK OpenGL mapper showed vertices only when both the vertex visibility
and edge visibility were turned on. That bug is now fixed.
