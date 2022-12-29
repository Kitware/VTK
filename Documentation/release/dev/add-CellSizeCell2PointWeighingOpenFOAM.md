## Add the ability to weigh the averaging of cell data to point data by cell size in the vtkOpenFOAMReader

The `vtkOpenFOAMReader` now provides a property `SizeAverageCellToPoint` that can be turned on to weigh the cell to point averaging operation (controlled itself by the `CreateCellToPoint` property) by cell size. This can be useful in cases where neighboring cells have very different sizes and therefore very different weight in the solution as a whole.

The default remains to not use any specific weighing of the values and perform a simple average over the neighboring cell values.
