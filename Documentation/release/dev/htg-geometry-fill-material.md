## Add FillMaterial option to HTG Geometry filter

`vtkHyperTreeGridGeometry` now has a `FillMaterial` option.
Enabled by default, the filter produces the same result as before.
When disabled, only the interface lines are added to the resulting polydata (nothing where there are no interfaces or on the edges).
