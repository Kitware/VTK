## Mesh Cache Usage

More VTK filters use vtkDataObjectMeshCache to optimize their
RequestData when input mesh did not change across execution.
The following filters were updated:
- vtkAppendDataSets
- vtkExtractGeometry
- vtkTransformFilter

### Developper notes
The vtkDataObjectMeshCache is not compatible with the vtkCompositeDataPipeline
main feature: it is intendend to handle the whole data, and not be used block per block.
To properly use a vtkDataObjectMeshCache, a filter should handle vtkCompositeDataSet
explicitly.

Note that it is a common limitation of vtkCompositeDataPipeline:
Two subsequent call to RequestData can be part of the same global Update (with different block)
or not. Relying on an information (like cache) from the previous run is then error prone.
