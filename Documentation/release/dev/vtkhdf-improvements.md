## Proper caching in vtkHDFReader

vtkHDFReader cache was reworked to work properly with all supported types but
vtkOverlappingAMR and vtkHyperTreeGrid, with added support
for vtkPartitionedDataSetCollection and vtkMultiBlockDataSet.

The caches now ensure the MeshMTime of vtkDataSet provided by
the vtkHDFReader do not change if they should not.

In this context, the UseCache member is now true by default
and it has been deprecated for further removal.

GetAttributeOriginalIdName and SetAttributeOriginalIdName have also been deprecated.

## Proper distributed support in vtkHDFReader

vtkHDFReader now generates proper vtkPartitionedDataSet distributed contents
with empty (nullptr) partitions where other ranks have data.
