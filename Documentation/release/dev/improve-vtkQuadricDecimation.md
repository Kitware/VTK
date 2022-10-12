## Add regularization option to vtkQuadricDecimation filter

The `vtkQuadricDecimation` filter now offers a new "regularized" mode that follows the algorithm exposed in [1] using a Gaussian distribution function.

Using this new mode is as simple as setting `vtkQuadricDecimation::SetRegularize(true)` and `vtkQuadricDecimation::SetRegularization(value)` to the `value` of the standard deviation to use for the distribution.

[1] P. Trettner and L. Kobbelt, Fast and Robust QEF Minimization using Probabilistic Quadrics, EUROGRAPHICS, Volume 39, Number 2 (2020)
