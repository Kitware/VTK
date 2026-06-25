## vtkXMLCompositeDataReader: Refactoring subclasses

All `vtkXMLCompositeDataReader` subclasses now call the following functions in `RequestInformation`

1. `PrepareToReadMetaData`
2. `CreateMetaData`
3. `SyncCompositeDataArraySelections`

The `Metadata` object is used to perform `CopyStructure`.

This refactoring allowed `vtkXMLPartitionedDataSetCollectionReader` to properly load the point/cell array selections.
Additionally, `vtkXMLUniformGridAMRReader` has been greatly simplified by subclassing
`vtkXMLPartitionedDataSetCollectionReader`.

Finally, `vtkXMLMultiBlockDataReader` and `vtkXMLPartitionedDataSetCollectionReader` now allow you select which
blocks to read via the `AddSelector/SetSelector/ClearSelectors` functions. It should be noted that by default
the Selector `/` is set to maintain backward compatibility, which means that all blocks will be read by default.
