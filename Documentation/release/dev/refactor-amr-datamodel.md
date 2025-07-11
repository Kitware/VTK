## Refactor of the AMR data model

The AMR data model has been refactored for futur proofing which incurs many deprecations:

- Deprecate AMR::Audit and add AMR::CheckValidity that returns a bool
- Deprecate vtkAMRInformation and introduce vtkOverlappingAMRMataData and vtkAMRMetaData
- Deprecate AMR::Set/GetData
- Deprecate AMR::GetInfo and introduce vtkUniformGridAMR::GetMetaData and vtkOverlappingAMR::GetOverlappingAMRMetaData
- Deprecate AMR::Initialize(nBlock, blocksPerLevel) into AMR::Initialize(const std::vector<unsigned int>& blocksPerLevel)
- Deprecate vtkUniformGridAMR::GetCompositeIndex into vtkUniformGridAMR::GetAbsoluteBlockIndex
- Deprecate vtkUniformGridAMR::GetLevelAndIndex into vtkUniformGridAMR::ComputeIndexPair
- Deprecate vtkAMRInformation::GetIndex into vtkAMRMetaData::GetAbsoluteBlockIndex
- Deprecate vtkAMRInformation::GetTotalNumberOfBlocks into vtkAMRMetaData::GetNumberOfBlocks
- Deprecate vtkAMRInformation::GetNumberOfDataSets(level) into vtkAMRMetaData::GetNumberOfBlocks(level)
- vtkUniformGridAMR::Bounds is now private
