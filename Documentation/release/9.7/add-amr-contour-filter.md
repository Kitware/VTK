## Add vtkAMRContourFilter

A vtkAMRContourFilter has been added.
It computes a perfectly watertight contour on vtkOverlappingAMR
by creating an interface between non-refined and refined grid of an AMR,
interpolating data on the interface wherever needed and then running contour on that interface.
The result is a vtkPartitionedDataSet of vtkPolyData.

Assumption:
 - A single voxel in a non-refined grid is not supposed to have points in common with grid of two other refinements levels.
 - A single edge should not be shared between grids of more than two different refinement level.

### Implementation details

- Iterate over each grid.
- If grid is of the highest refinement, just run a contour filter on it.
- If not, identify the "interface", which are cells that are neighbors with a more refined cell.
- Blank interface cells and keep a note of them, then run contour filter on the non-refined grid.
- Split each interface cell into multiple unstructured cells to create an actual interface between high resolution grid and low resolution grid.
- Interpolate data on the interface wherever needed.
- Run the contour on the interface.
- Put each contour part in a partitioned dataset, this is the output.

The complex part is obviously the splitting of low resolution grid cells into interface cells,
The idea is to create pyramids and tetrahedrons out of the cells, by adding points on the edges (from the refined grid)
as well as in the center of the faces and center of the voxel.

- Iterate over each cells of the interface.
- Check each edges of the cell and store the number of "split points", 2 means not split, more mean split.
- Check each face of the cell.
 - If no edges are split, the face is facing a non-refined grid, then create a simple pyramid using the 4 face point and the voxel center.
 - If all edges are split (equally), the face is facing a refined grid, recover the refined point of the refined grid and create many pyramids between these quads and the voxel center.
 - Else, its an "interface" face, create tetrahedrons beween each "split" edges, the face center and the voxel center.

This creates a perfect paving of the interface, connecting the refined grid with the non refined grid.
The data on the interface points matters for the contour. We obviously first take data from the refined grid when available, then data from the non-refined grid.
If no data is available (interface face centers and voxel center), then a simple shepherd interpolation is used using all the points on the face or the voxel respectively.


Rough face:

![image](../imgs/add-amr-contour-filter_rough.png)

Refined face

![image](../imgs/add-amr-contour-filter_refined.png)

Interface face:

![image](../imgs/add-amr-contour-filter_interface.png)
