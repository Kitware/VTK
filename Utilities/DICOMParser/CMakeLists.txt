

ADD_LIBRARY(vtkDICOMParser STATIC DICOMFile.cxx DICOMParser.cxx DICOMAppHelper.cxx)
INSTALL_TARGETS(/lib/VTK vtkDICOMParser)
INSTALL_FILES(/include/VTK/Utilities/DICOMParser "(\\.h|\\.txx)$")
