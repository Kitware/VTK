## Arbitrary-order and Bernstein-Bezier bases for cell-grids

Cell-grid attributes on discontinuous Galerkin cells may now use any polynomial
order. `HGRAD` provides Lagrange bases at every order for every cell shape but
the pyramid. Hexahedra and quadrilaterals accept an independent order along
each parametric axis, and wedges an independent order for their triangular
cross-section and their axial direction, through an array in the `order` role.

The new `Bezier` function space provides Bernstein-Bezier bases for the same
shapes and orders. They span the same polynomials as `HGRAD` but take control
values as their degrees of freedom, so a cell's values stay within the convex
hull of those values.

The new `vtkCellGridChangeBasis` filter rewrites an attribute's coefficients
from one basis to another, such as `HGRAD` to `Bezier`, without changing the
function they describe. The target may also raise the polynomial order.

The new `vtkLagrangePoints` attribute calculator reports where a basis places
each of its degrees of freedom in parameter space.

This also corrects the quadratic edge basis functions, which did not sum to one
and whose gradient left one function uninitialized, and `vtkMath::Binomial()`,
which could return a result one less than the true coefficient.
