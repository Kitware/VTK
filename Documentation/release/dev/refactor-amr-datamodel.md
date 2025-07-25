## Refactor of the AMR data model

The AMR data model has been refactored for future proofing.
vtkUniformGridAMR now inherits from vtkPartitionedDataSetCollection instead of vtkCompositeDataSet.

This lets classes interact with AMR data as they would with any data object tree or pdc when accessing data,
without requiring specific code. Specific code can and will still be needed in certain cases.
Of course, when setting up the AMR, the AMR API should be used.

Indeed, each AMR level is now stored as a vtkPartitionedDataSet,
and the AMR structure itself is described in the vtkAMRMetaData class stored internally.

Another impactful change was the separation of vtkAMRInformation into vtkAMRMetaData and vtkOverlappingAMRMetaData.
This allow much simpler handling of the Overlapping vs NonOverlapping case and clear separation of
responsibility.

In the process many API were reworked for consistency and stability.
Backward compatibility was kept for many methods but certains became redundant and are just no-op
with a deprecation flag.

Here is the exhaustive list.
`AMR::` refer to any AMR class, new or already present, for the sake of brevity.
(vtkUniformGridAMR, vtkNonOverlappingAMR, vtkOverlappingAMR, vtkAMRInformation, vtkAMRMetaData, vtkOverlappingAMRMetaData)

Classes:

- Deprecate vtkAMRInformation and introduce vtkOverlappingAMRMataData and vtkAMRMetaData
- Deprecate vtkAMRDataInternals which is not used anymore
- Deprecate vtkUniformGridAMRDataIterator into vtkUniformGirdAMRIterator

Methods:

- Deprecate AMR::Audit and add AMR::CheckValidity that returns a bool
- Deprecate AMR::Set/GetData, which are currently no-op.
- Deprecate AMR::GetInfo and introduce vtkUniformGridAMR::GetMetaData and vtkOverlappingAMR::GetOverlappingAMRMetaData
- Deprecate AMR::Initialize(nBlock, blocksPerLevel) into AMR::Initialize(const std::vector<unsigned int>& blocksPerLevel)
- Deprecate vtkUniformGridAMR::GetCompositeIndex into vtkUniformGridAMR::GetAbsoluteBlockIndex
- Deprecate vtkUniformGridAMR::GetLevelAndIndex into vtkUniformGridAMR::ComputeIndexPair
- Deprecate vtkAMRInformation::GetIndex into vtkAMRMetaData::GetAbsoluteBlockIndex
- Deprecate vtkAMRInformation::GetTotalNumberOfBlocks into vtkAMRMetaData::GetNumberOfBlocks
- Deprecate vtkAMRInformation::GetNumberOfDataSets(level) into vtkAMRMetaData::GetNumberOfBlocks(level)

Members:

- vtkUniformGridAMR::Bounds is now private

Change of behavior:

- vtkUniformGridAMR now inherits vtkPartitionedDataSetCollection, which means any vtkPartitionedDataSetCollection::SafeDownCast(amr)
  will not return nullptr anymore.

- vtkUniformGridAMRIterator now inherits vtkDataObjectTreeIterator, which means any vtkDataObjectTreeIterator::SafeDownCast(amrIter)
  will not return nullptr anymore.

- vtkPlaneCutter now outputs a PDC instead of a MBD with a AMR input.

- vtkDataObjectTree::CopyStructure(amr) used to reconstruct a data object tree with an simili AMR structure,
  this is not the case anymore and it just copy the partitioned dataset collection structure using the generic implementation.

- vtkUniformGridAMR::CopyStructure(amr) now copy the bounds as well to ensure retro compatibility
  when dealing with vtkNonOverlappingAMR that do not contains bounds in the vtkAMRMetaData, contrary to vtkOverlappingAMR.

- vtkExtractBlockUsingDataAssembly now outputs a vtkPartitionedDataSetCollection when input is a vtkNonOverlappingAMR for consistency.
