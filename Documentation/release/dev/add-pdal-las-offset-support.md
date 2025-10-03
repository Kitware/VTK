## Add support for LAS offsets in vtkPDALReader

vtkPDALReader can now read X/Y/Z offset values from PDAL metadata.
This improves handling of georeferenced LAS/LAZ files by applying
the proper offset without manual configuration.
