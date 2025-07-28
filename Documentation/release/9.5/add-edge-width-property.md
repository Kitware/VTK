# Add option to independently set thickness of cell edges and lines

You can now set the thickness of cell edges independently without altering the
thickness of lines. This feature is useful to emphasize either edges or lines in a scene
with different widths.

The `vtkProperty` class now offers two new options to deal with edges:

1. `vtkProperty::SetEdgeWidth(float)` controls the thickness of cell edges.
2. `vtkProperty::SetUseLineWidthForEdgeThickness(bool)` specifies whether the thickness of edges is equal the value of `LineWidth` option (default) or the `EdgeWidth` option (new)

The default value of `UseLineWidthForEdgeThickness` is `true` to prevent breaking the behaviour of existing applications. Applications that used `LineWidth` to control the thickness of edges will continue to work as expected.

As an example, here is a single polydata that has both polygonal faces and polylines. Notice how the edges are wider than the lines. You can achieve this by configuring `EdgeWidth` and
turning off `UseLineWidthForEdgeThickness` on the actors' `vtkProperty` instance.

![add-edge-width-property.png](../imgs/9.5/add-edge-width-property.png)
