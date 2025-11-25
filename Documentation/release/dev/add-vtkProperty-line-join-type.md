# Extend vtkProperty with a new line join setting

You can now set the kind of join geometry to use between contiguous line segments of a polyline with
the `vtkProperty::SetLineJoin(LineJoinType)` method. The possible values for the `LineJoinType` enum
are:
1. RoundCapRoundJoin
2. MiterJoin
3. NoJoin (default)

This feature is only supported in the upcoming experimental webgpu rendering backend.
