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

## Piece distribution configuration in vtkHDFReader

When reading partitioned data in parallel, you can now use `vtkHDFReader::SetPieceDistribution`
to choose the piece distribution strategy: interleave blocks between processes, or allocate "blocks" of partitions.

## Better support for partitioned data writing in vtkHDFWriter

`vtkHDFWriter` is now able to write partitioned data in parallel
when the number of non-null partitions differ between processes.

## Removal of unspecified Temporal FieldData behavior with vtkOverlappingAMR

vtkOverlappingAMR does not specify properly how to handle temporal field data
in the VTKHDF specifications, the implementation have been removed before a proper
reimplementation.

## Quiet support in vtkHDFUtilities::Open

A new argument have been added to vtkHDFUtilities::Open
to suppress all console output on error. The previous version has been deprecated.

## vtkHyperTreeGrid support in vtkHDFWriter

`vtkHDFWriter` now properly supports the HyperTreeGrid data model. It can write partitioned, temporal and distributed data.

## vtkRectilinearGrid & vtkStructuredGrid support

The VTKHDF specification now supports Recilinear Grids and Structured Grids.

`vtkHDFWriter` and `vtkHDFReader` are both capable of handling these new datatypes, for simple and time-dependent data.
