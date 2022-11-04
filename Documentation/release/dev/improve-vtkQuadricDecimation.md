## Add regularization option to vtkQuadricDecimation filter

The `vtkQuadricDecimation` filter now offers a new "regularized" mode that follows the algorithm exposed in [1] using a Gaussian distribution function.

Using this new mode is as simple as setting `vtkQuadricDecimation::SetRegularize(true)` and `vtkQuadricDecimation::SetRegularization(value)` to the `value` of the standard deviation to use for the distribution.

Additionally, there is a change in the way the filter defaults to handling the weighting of the boundary constraints. The weights now default to units of area instead of length to be more homogeneous with the rest of the algorithm. In order to obtain the previous behavior one may set the `LegacyBoundaryWeighting` property to `true`.

[1] P. Trettner and L. Kobbelt, Fast and Robust QEF Minimization using Probabilistic Quadrics, EUROGRAPHICS, Volume 39, Number 2 (2020)
