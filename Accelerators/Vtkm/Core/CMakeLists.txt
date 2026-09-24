list(INSERT CMAKE_MODULE_PATH 0
  "${VTK_SOURCE_DIR}/ThirdParty/viskores/vtkviskores/CMake")

set(private_headers
  vtkmlib/DataArrayConverters.hxx
  vtkmlib/Portals.hxx
  vtkmlib/PortalTraits.h)

set(classes
  vtkmDataArrayFactory)

set(nowrap_classes
  vtkmDataArray
  vtkmDataArrayUtilities
  vtkmlib/DataArrayConverters
  vtkmlib/DataSetUtils
  vtkmlib/Portals
  vtkmlib/vtkmInitializer)

set(nowrap_headers
  vtkmlib/vtkmDataArray.hxx)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfigCore.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigCore.h"
  @ONLY)

set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigCore.h")

vtk_module_add_module(VTK::AcceleratorsVTKmCore
  HEADER_DIRECTORIES
  SOURCES ${sources}
  HEADERS ${headers}
  CLASSES ${classes}
  NOWRAP_HEADERS  ${nowrap_headers}
  NOWRAP_CLASSES  ${nowrap_classes}
  PRIVATE_HEADERS ${private_headers})
vtk_module_set_property(VTK::AcceleratorsVTKmCore
  PROPERTY  JOB_POOL_COMPILE
  VALUE     viskores_pool)

_vtk_module_real_target(vtkm_accel_target VTK::AcceleratorsVTKmCore)
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
  vtk_module_compile_options(VTK::AcceleratorsVTKmCore
    PRIVATE
      ${viskores_msvc_flags})
endif ()
