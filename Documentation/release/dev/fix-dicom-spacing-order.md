# Fix ordering of DICOM pixel spacing

The vtkDICOMImageReader used to provide the vertical and horizontal pixel
spacing in the wrong order. This issue was only discovered recently because
DICOM pixels are nearly always square.
