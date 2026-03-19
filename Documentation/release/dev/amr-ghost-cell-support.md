## AMR ghost cell support and blanking fixes

### Ghost cell support in vtkAMReXGridReader

`vtkAMReXGridReader` now reads ghost cells from AMReX plotfiles that were
written with `nGrow > 0`. When ghost cells are present in the plotfile, the
reader creates grids that are larger than the AMR box by `nGrow` cells on each
side. The grid extent uses the convention `[-nGrow, validCells + nGrow]` per
dimension so that extent index 0 corresponds to the AMR box lo corner. A
`vtkGhostType` array is generated marking ghost cells with the `DUPLICATECELL`
flag.

Previously, ghost cells in AMReX plotfiles were not supported (marked with a
TODO comment in the code). Reading a plotfile with ghost cells could result in
data misalignment or crashes because the FAB data on disk was larger than the
grid allocated by the reader.

### AMR cell blanking fix in vtkAMRUtilities

`vtkAMRUtilities::BlankGridsAtLevel` now correctly handles grids with ghost
cells when computing cell blanking (the `REFINEDCELL` flag). The blanking code
converts AMR global coordinates to extent coordinates and uses
`vtkStructuredData::ComputeCellIdForExtent`, which correctly accounts for grids
whose extent does not start at zero. Previously, the blanking code used
`vtkAMRBox::GetCellLinearIndex` which assumed the grid dimensions matched the
AMR box dimensions, producing incorrect cell IDs when ghost cells were present.

### Parent-child relationship fix in vtkOverlappingAMRMetaData

`vtkOverlappingAMRMetaData::CalculateParentChildRelationShip` now correctly
finds parent-child relationships when the number of ghost cells is not a
multiple of the refinement ratio. The spatial binner used for accelerating
parent-child detection now clamps bin indices to valid ranges using signed
arithmetic. Previously, when a child box at the fine level extended beyond the
refined parent box bounds (which happens when `nGrow < refinementRatio`), the
unsigned arithmetic in the bin index computation could wrap around, causing the
binner to miss valid parent-child relationships entirely. This resulted in no
blanking being applied.
