# Fix vtkCompositePolyDataMapper opacity override

You can now render opaque blocks in a composite dataset even when the actor has an opacity < 1.0 by setting
the opacity of the block equal to 1.0. Previously, a bug in vtkActor prevented the overridden block's opacity
value from being considered when the actor was transparent.

![Screenshot showing an opaque block in an actor that is transparent](../imgs/9.5/fix-composite-mapper-opacity-override.png)
