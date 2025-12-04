## vtkArrayCalculator: Enable vector definition of arbitrary size

`vtkArrayCalculator` now supports user-defined vectors of arbitrary size using the `{x, y, z, ...}` syntax and **NOTHING
ELSE** before or after the curly braces. The number of elements inside the braces determines the number of components in
the resulting array.

Additionally, the `pi` and `inf` constants are now added.

Finally, when the input is `vtkHyperTreeGrid`, the default attribute is correctly set to `vtkDataObject::CELL`.
