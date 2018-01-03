if (WIN32)
  if (BUILD_SHARED_LIBS)
    set(DICOM_DLL 1)
  else ()
    set(DICOM_STATIC 1)
  endif ()
endif ()
set(DICOM_ANSI_STDLIB 1)
configure_file(DICOMCMakeConfig.h.in DICOMCMakeConfig.h)

set(headers
  DICOMAppHelper.h
  DICOMCallback.h
  DICOMConfig.h
  DICOMFile.h
  DICOMParser.h
  DICOMParserMap.h
  DICOMTypes.h
  "${CMAKE_CURRENT_BINARY_DIR}/DICOMCMakeConfig.h")

set(sources
  DICOMAppHelper.cxx
  DICOMFile.cxx
  DICOMParser.cxx)

# FIXME: DICOMParser only exports symbols properly on Windows.
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_add_module(VTK::DICOMParser
  SOURCES ${sources}
  HEADERS ${headers})
vtk_module_compile_features(VTK::DICOMParser
  PUBLIC
    cxx_std_11)
