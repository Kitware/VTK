## Proper caching in vtkHDFReader

vtkHDFReader cache was reworked to work properly with all supported types but
vtkHyperTreeGrid, with added support for vtkPartitionedDataSetCollection, vtkMultiBlockDataSet and vtkOverlappingAMR.

The caches now ensure the MeshMTime of vtkDataSet provided by
the vtkHDFReader do not change if they should not.

In this context, the UseCache member is now true by default
and it have been deprecated for further removal.

GetAttributeOriginalIdName, SetAttributeOriginalIdName and AddFieldArrays have also been deprecated.

vtkHDFUtilities::RetrieveHDFInformation have been deprecated in favor of a version with more arguments.

## Proper distributed support in vtkHDFReader

vtkHDFReader now generates proper vtkPartitionedDataSet distributed contents
with empty (nullptr) partitions where other ranks have data.

## Removal of unspecified Temporal FieldData behavior with vtkOverlappingAMR

vtkOverlappingAMR does not specify properly how to handle temporal field data
in the VTKHDF specifications, the implementation have been removed before a proper
reimplementation.
