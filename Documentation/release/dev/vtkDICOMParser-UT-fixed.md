## Fixed UT record support in vtkDICOMParser (!11149)

Previously, vtkDICOMParser failed to parse DICOM files with UT records
due to incorrectly reading their length field. This has now been fixed.
