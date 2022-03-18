# Improve vktHyperTreeGrid support

## Add support for vtkHyperTreeGrid
We improve the support of the HyperTreeGrid data model in the following use cases:
 * `vtkArrayCalculator` filter
 * numpy wrapping with `dataset_adapter` module
 * `vtkProgrammableFilter`

## API Reword for consistency
HyperTreeGrid model relies on graph, thus initially the term of Vertices was used in its API.
But in the VTK world, HyperTreeGrid vertices are more like Cells.
Moreover 'Vertices' also has another meaning in VTK.
So, we reword vtkHyperTreeGrid `GetNumberOfVertices` to `GetNumberOfCells`, for consistency.
