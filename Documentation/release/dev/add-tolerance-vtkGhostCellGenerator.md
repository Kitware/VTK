## vtkGhostCellGenerator: Add Tolerance Parameter

`vtkGhostCellGenerator` provides three distinct paths for identifying ghost cells:

1. Utilize the global point IDs present in your dataset
2. If global point IDs were not present:
    1. Generate them on the fly using `vtkGenerateGlobalIds`.
    2. Use a `vtkAbstractPointLocator` to match coincident points.

## Problem

Previously, these two paths when global point IDs were not present applied inconsistent tolerances, leading to divergent
ghost results:

* The `vtkGenerateGlobalIds` path enforced a strict **zero tolerance**.
* The point locator path utilized a dynamic, value-dependent precision tolerance based on point coordinates (scaling
  with `epsilon`).

## Solution

This update resolves the discrepancy by introducing a configurable `Tolerance` parameter to `vtkGhostCellGenerator`,
ensuring identical and predictable results across both paths.

* **Default Value:** The default tolerance is set to `std::numeric_limits<double>::epsilon()`. A strict zero-tolerance
  default was avoided to prevent potential matching issues when datasets contain varying point precisions.
* **Special Note:** If your dataset contains topologically distinct points with bit-identical coordinates that need to
  be strictly separated, you can explicitly configure a zero tolerance.
