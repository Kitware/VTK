set(Viskores_NO_DEPRECATED_VIRTUAL ON)

list(INSERT CMAKE_MODULE_PATH 0
  "${VTK_SOURCE_DIR}/ThirdParty/viskores/vtkviskores/CMake")

set(sources
  vtkmlib/ArrayConvertersReal.cxx
  vtkmlib/ArrayConvertersSigned.cxx
  vtkmlib/ArrayConvertersUnsigned.cxx)

set(private_headers
  vtkmlib/ArrayConverters.hxx)

set(nowrap_classes
  vtkmlib/ArrayConverters
  vtkmlib/CellSetConverters
  vtkmlib/DataSetConverters
  vtkmlib/ImageDataConverter
  vtkmlib/ImplicitFunctionConverter
  vtkmlib/PolyDataConverter
  vtkmlib/UnstructuredGridConverter)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfigDataModel.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigDataModel.h"
  @ONLY)

list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigDataModel.h")

vtk_module_add_module(VTK::AcceleratorsVTKmDataModel
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  NOWRAP_CLASSES  ${nowrap_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_set_property(VTK::AcceleratorsVTKmDataModel
  PROPERTY  JOB_POOL_COMPILE
  VALUE     viskores_pool)
_vtk_module_real_target(vtkm_accel_target VTK::AcceleratorsVTKmDataModel)
vtk_add_viskores_device_target_information(${vtkm_accel_target})

if (MSVC)
  set(msvc_warning_flags
    # Generates numerous false positives with template code about unreachable
    # code
    /wd4702
    # Generates numerous warning that implicit assignment operators can't be
    # constructed. This is understood and we don't care.
    /wd4512
    # Generates numerous warning that implicit constructors can't be
    # constructed. This is understood and we don't care.
    /wd4510
    # Generates warnings when decorated names are over 4096 characters
    # This compiler warning is deprecated and no longer occurs with VS2017+.
    # So we suppress for VS2015
    /wd4503
    )
  set(viskores_msvc_flags)
  foreach (msvc_warning_flag IN LISTS msvc_warning_flags)
    if (TARGET VTK::vtkviskores_cuda)
      list(APPEND viskores_msvc_flags
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${msvc_warning_flag},${msvc_warning_flag}>)
    else ()
      list(APPEND viskores_msvc_flags
        ${msvc_warning_flag})
    endif ()
  endforeach ()
  vtk_module_compile_options(VTK::AcceleratorsVTKmDataModel
    PRIVATE
      ${viskores_msvc_flags})
endif ()
