# Add vtkHyperTreeGridGradient

A new HyperTreeGrid algorithm has been introduced, computing the gradient
of a given scalar field. The computation rely on the edges of the dual and
as so, takes all neighbors into account. Coarse cells are ignored.
