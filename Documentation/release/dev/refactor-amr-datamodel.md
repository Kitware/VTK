## Refactor of the AMR data model

The AMR data model has been refactored for futur proofing which incurs many deprecations:

- Deprecate AMR::Audit and add AMR::CheckValidity that returns a bool
- Deprecate vtkAMRInformation and introduce vtkOverlappingAMRMataData and vtkAMRMetaData
- Deprecate AMR::Set/GetData
- Deprecate AMR::GetInfo and introduce vtkUniformGridAMR::GetMetaData and vtkOverlappingAMR::GetOverlappingAMRMetaData
- vtkUniformGridAMR::Bounds is now private
