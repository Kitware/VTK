set(Viskores_NO_DEPRECATED_VIRTUAL ON)

list(INSERT CMAKE_MODULE_PATH 0
  "${VTK_SOURCE_DIR}/ThirdParty/viskores/vtkviskores/CMake")

set(classes
  vtkmAverageToCells
  vtkmAverageToPoints
  vtkmCleanGrid
  vtkmClip
  vtkmContour
  vtkmCoordinateSystemTransform
  vtkmExternalFaces
  vtkmExtractVOI
  vtkmGradient
  vtkmHistogram
  vtkmImageConnectivity
  vtkmLevelOfDetail
  vtkmNDHistogram
  vtkmPointElevation
  vtkmPointTransform
  vtkmPolyDataNormals
  vtkmProbe
  vtkmSlice
  vtkmThreshold
  vtkmTriangleMeshPointNormals
  vtkmWarpScalar
  vtkmWarpVector
  )

set(sources
  vtkmClipInstantiationsWithField.cxx
  vtkmClipInstantiationsWithImplicitFunction.cxx)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmConfigFilters.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigFilters.h"
  @ONLY)

set(headers
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmConfigFilters.h")

#=============================================================================
# Option to enable Viskores override of corresponding VTK filters. Note that
# there is also a run-time option that needs to be enabled. It can be accessed
# via the static function `vtkmFilterOverrides::SetEnabled(bool)`
#
set(override_default OFF)
if (DEFINED VTK_ENABLE_VTKM_OVERRIDES)
  set(override_default "${VTK_ENABLE_VTKM_OVERRIDES}")
  unset(VTK_ENABLE_VTKM_OVERRIDES CACHE)
  message(DEPRECATION
  "The `VTK_ENABLE_VTKM_OVERRIDES` option has been replaced by `VTK_ENABLE_VISKORES_OVERRIDES`")
endif ()
option(VTK_ENABLE_VISKORES_OVERRIDES "Override VTK filters using the object factory" "${override_default}")
mark_as_advanced(VTK_ENABLE_VISKORES_OVERRIDES)

macro (_viskores_add_override override_base override_sub)
  vtk_object_factory_declare(BASE ${override_base} OVERRIDE ${override_sub})
  list (APPEND VISKORES_OVERRIDES_INITIALIZER_LIST "{\"${override_base}\", \"${override_sub}\"}")
endmacro()

if (VTK_ENABLE_VISKORES_OVERRIDES)
  _viskores_add_override("vtkContourFilter" "vtkmContour")
  _viskores_add_override("vtkGradientFilter" "vtkmGradient")
  _viskores_add_override("vtkCellDataToPointData" "vtkmAverageToPoints")
  _viskores_add_override("vtkPointDataToCellData" "vtkmAverageToCells")
  _viskores_add_override("vtkTableBasedClipDataSet" "vtkmClip")
  _viskores_add_override("vtkCutter" "vtkmSlice")
  _viskores_add_override("vtkThreshold" "vtkmThreshold")

  list (JOIN VISKORES_OVERRIDES_INITIALIZER_LIST ",\n  " VISKORES_OVERRIDES_INITIALIZER_LIST_STRING)

  vtk_object_factory_configure(
    SOURCE_FILE vtk_object_factory_source
    HEADER_FILE vtk_object_factory_header
    EXPORT_MACRO "VTKACCELERATORSVTKMFILTERS_EXPORT")

  list (APPEND sources ${vtk_object_factory_source})
  list (APPEND headers ${vtk_object_factory_header})

  # Set this here instead of the vtk.module file as it is conditional
  set_property(GLOBAL APPEND
    PROPERTY
    "_vtk_module_VTK::AcceleratorsVTKmFilters_implements" VTK::FiltersCore)
endif()

set (VISKORES_OVERRIDES_INITIALIZER_LIST_STRING "{\n  ${VISKORES_OVERRIDES_INITIALIZER_LIST_STRING}};")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkmFilterOverrides.cxx.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkmFilterOverrides.cxx")
list (APPEND headers vtkmFilterOverrides.h)
list (APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/vtkmFilterOverrides.cxx")
#=============================================================================

vtk_module_add_module(VTK::AcceleratorsVTKmFilters
  SOURCES ${sources}
  HEADERS ${headers}
  CLASSES ${classes})
vtk_module_set_property(VTK::AcceleratorsVTKmFilters
  PROPERTY  JOB_POOL_COMPILE
  VALUE     viskores_pool)
vtk_module_definitions(VTK::AcceleratorsVTKmFilters
  PUBLIC "VTK_ENABLE_VISKORES_OVERRIDES=$<BOOL:${VTK_ENABLE_VISKORES_OVERRIDES}>")

_vtk_module_real_target(vtkm_accel_target VTK::AcceleratorsVTKmFilters)
viskores_add_target_information(${vtkm_accel_target}
                            EXTENDS_VISKORES
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ${sources})

if (TARGET viskores::cuda)
  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")

  list(TRANSFORM classes APPEND ".cxx" OUTPUT_VARIABLE cuda_impl)
  set_source_files_properties(${cuda_impl} PROPERTIES LANGUAGE CUDA)
  vtk_module_set_properties(VTK::AcceleratorsVTKmFilters CUDA_SEPARABLE_COMPILATION ON)

  vtk_module_compile_options(VTK::AcceleratorsVTKmFilters
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
elseif (TARGET viskores::kokkos_cuda)

  list(TRANSFORM classes APPEND ".cxx" OUTPUT_VARIABLE cuda_impl)
  set_source_files_properties(${cuda_impl} PROPERTIES LANGUAGE CUDA)
  kokkos_compilation(SOURCE ${cuda_impl})

endif()

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
    if (TARGET viskores::cuda OR TARGET viskores::kokkos_cuda)
      list(APPEND viskores_msvc_flags
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${msvc_warning_flag},${msvc_warning_flag}>)
    else ()
      list(APPEND viskores_msvc_flags
        ${msvc_warning_flag})
    endif ()
  endforeach ()
  vtk_module_compile_options(VTK::AcceleratorsVTKmFilters
    PRIVATE
      ${viskores_msvc_flags})
endif ()
