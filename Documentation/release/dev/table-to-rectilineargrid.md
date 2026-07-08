## New vtkTableToRectilinearGrid filter

The `vtkTableToRectilinearGrid` filter can generate `vtkRectilinearGrid` from `vtkTable` input.

The filter uses 3 given columns as `X`, `Y` and `Z` coordinates (see `SetXYZColumn()`).
The other columns are used as PointData.
Some points may be missing in the input table: they are mark as blank in the grid.
(see vtkRectilinearGrid::BlankPoint), as long as the cells they belong too.

Unlike `vtkTableToStructuredGrid`, the table does not need to respect any order.

### Example

Lets have this table for a 2D example
```
A, B, C
0, 0, 10
1, 1, 11
0, 1, 12
0, 2, 13
1, 2, 14
```
Using A as X and B as Y leads to this grid, with C value displayed at point position:
```
   G   ─  ─  11───────14
             │        │
   |  Blank  │        │
             │        │
   10  ─  ─  12───────13
```
explanation:

- A has values in (0, 1): X dimension is 2
- B has values in (0, 1, 2): Y dimensions is 3
- Looking for a (a, b) tuple in the rows give us a PointData for the (a, b) point
- The table does not contain a row where A=1 and B=0. So the point G is a blank point. Thus the cell 0 (on the left) is blanked too.
