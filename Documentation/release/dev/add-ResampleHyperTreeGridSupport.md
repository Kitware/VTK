## Pain Point

Previously, the `vtk(P)ResampleWithDataSet` filters did not support using `vtkHyperTreeGrid`s as sources for
resampling.

## Feature

Add support for `vtkHyperTreeGrid` in the resampling filters using the `vtkHyperTreeGridProbeFilter`.

## Steps

- refactor exisiting probe functionality into sequential and parallel parts
- insert support for `vtkHyperTreeGrid` into both sequential and parallel resampling filters
