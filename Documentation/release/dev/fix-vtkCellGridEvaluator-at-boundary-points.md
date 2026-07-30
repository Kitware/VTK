# Add a classifier tolerance for vtkCellGridEvaluator

`vtkCellGridEvaluator` gained a `ClassifierTolerance` property: the parametric-space
tolerance used to accept points near the reference-element boundary during classification.
With the default (0.0), containment is tested as strictly as the Newton solver's own
convergence precision allows; set a small positive value (e.g. `1e-8`) when probe points
lie on boundaries shared between neighboring cells so that rounding in the iteration does
not exclude them from one of the cells. Classification convergence is now also measured
in parametric space (the Newton step mapped through the inverse Jacobian), making it
independent of the world-space size of the cells.
