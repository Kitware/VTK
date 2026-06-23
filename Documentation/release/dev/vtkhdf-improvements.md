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

## New data types specified: vtkRectilinearGrid, vtkStructuredGrid and vtkTable

The VTKHDF specification now supports Recilinear Grids, Structured Grids and Tables.

`vtkHDFWriter` and `vtkHDFReader` are both capable of handling these new datatypes, for simple and time-dependent data (no partition or extent support).

## vtkHDFUtilities and vtkHDFReader deprecations

- The protect method `vtkHDFReader::DataArraySelection` now returns a `std::map` instead of a `std::array` with `vtkDataObject::AttributeTypes::` as a key.
- `vtkHDFUtilities::GetArrayNames`, `RetrieveHDFInformation` and `NewFieldArray` also use a `std::map<int, hid_t>` for the `attributeDataGroup` argument instead of a `std::array<hid_t, 3>`.
