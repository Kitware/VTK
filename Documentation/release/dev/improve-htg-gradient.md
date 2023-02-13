# Bring vector support and new computations to the HTG gradient filter

The `vtkHyperTreeGridGradient` filter is now able to handle vector fields.
The resulting gradient has 3 times more components than the input.

Additionally, for vector input we can now compute the **vorticity**,
**divergence** and **Q-Criterion** as possible in the existing generic filter.
