# Add vtkMergeVectorComponents Filter

This filter:
* is a VTK Filter under the Filters/General module
* It is a subclass of **vtkPassInputTypeAlgorithm** (currently supporting only vtkDataset)
* The classes **vtkTableToPolyData** and **vtkExtractVectorComponents** were used as starting points.
* The filter has **five** functions:
    * Three functions to set the array names of the x, y, and z components
    * One function to set the output array name. If no name is set, then "combinationVector" is used by default.
    * One function to set the AttributeType of the arrays. **AttributeTypes** were used instead of **FieldAssociations** because PointData and CellData are extracted using the function **GetAttributesAsFieldData** which utilizes AttributeTypes internally. Only the following are currently supported:
        * vtkDataObject::POINT, for PointData,
        * vtkDataObject::CELL, for CellData
* The output array type is currently **vtkDoubleArray**.
* It should be noted that, the output array is not as ActiveVector by the filter.
* A test has also been developed which creates a sphere and:
    * adds point-related arrays, uses the filter, and compares the input arrays with the output of the filter
    * adds cell-related arrays, uses the filter, and compares the input arrays with the output of the filter
