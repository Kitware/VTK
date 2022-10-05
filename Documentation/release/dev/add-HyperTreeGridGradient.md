# Add vtkHyperTreeGridGradient

The `vtkHyperTreeGridGradient` class is a Gradient filter specialized to
process `vtkHyperTreeGrid` datasets. This filter has two modes:
* UNLIMITED that computes the gradient with Unlimited cursors, refining
  neighboring nodes to have a local computation similar to a regular grid;
* UNSTRUCTURED that computes the gradient with default cursors, giving
  similar results than an unstructured grid computation.

In unlimited mode, it is possible to handle extensive attributes so the
virtual subdivision reduces their influence.
