## Fix vtkDICOM remote module naming inconsistency

Fixed a long-standing naming inconsistency in the `vtkDICOM` remote module. Previously, enabling the module via CMake produced "unused variable" warnings and required running CMake twice. The remote module fetching name now correctly aligns with the internal module definition (`DICOM`), allowing the standard flag `-DVTK_MODULE_ENABLE_VTK_DICOM=YES` to work correctly on the first configuration pass.
