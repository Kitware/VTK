option(VTK_DISPATCH_AFFINE_ARRAYS "Include implicit vtkDataArray subclasses based on an affine function backend in dispatcher" OFF)
option(VTK_DISPATCH_COMPOSITE_ARRAYS "Include implicit vtkDataArray subclasses based on a composite binary tree backend in dispatcher" OFF)
option(VTK_DISPATCH_CONSTANT_ARRAYS "Include implicit vtkDataArray subclasses based on a constant backend in dispatcher" OFF)
option(VTK_DISPATCH_INDEXED_ARRAYS "Include implicit vtkDataArray subclasses based on an index referencing backend in dispatcher" OFF)
option(VTK_DISPATCH_STD_FUNCTION_ARRAYS "Include implicit vtkDataArray subclasses based on std::function in dispatcher" OFF)
mark_as_advanced(
  VTK_DISPATCH_AFFINE_ARRAYS
  VTK_DISPATCH_COMPOSITE_ARRAYS
  VTK_DISPATCH_CONSTANT_ARRAYS
  VTK_DISPATCH_INDEXED_ARRAYS
  VTK_DISPATCH_STD_FUNCTION_ARRAYS
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"
  @ONLY)


include("${CMAKE_CURRENT_SOURCE_DIR}/vtkCreateArrayDispatchImplicitList.cmake")
vtkArrayDispatchImplicit_default_array_setup()
vtkArrayDispatchImplicit_generate_array_header(VTK_ARRAYDISPATCH_IMPLICIT_ARRAY_LIST)
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkArrayDispatchImplicitArrayList.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkArrayDispatchImplicitArrayList.h"
  @ONLY)

set(instantiation_sources)
foreach (INSTANTIATION_VALUE_TYPE IN LISTS vtkArrayDispatchImplicit_all_types)
  # only instantiate fundamental types, not typedefs
  if (INSTANTIATION_VALUE_TYPE STREQUAL "vtkIdType")
    continue()
  endif ()
  # create a file suffix from the type
  string(REPLACE " " "_" _suffix "${INSTANTIATION_VALUE_TYPE}")

  list(APPEND _list "vtkAffineArrayInstantiate")
  list(APPEND _list "vtkCompositeArrayInstantiate")
  list(APPEND _list "vtkCompositeImplicitBackendInstantiate")
  list(APPEND _list "vtkConstantArrayInstantiate")
  list(APPEND _list "vtkIndexedArrayInstantiate")
  list(APPEND _list "vtkIndexedImplicitBackendInstantiate")
  list(APPEND _list "vtkStdFunctionArrayInstantiate")

  # generate cxx file to instantiate template with this type
  foreach (_prefix IN LISTS _list)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/${_prefix}.cxx.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${_prefix}_${_suffix}.cxx"
      @ONLY)
    list(APPEND instantiation_sources
      "${CMAKE_CURRENT_BINARY_DIR}/${_prefix}_${_suffix}.cxx")
  endforeach ()
endforeach ()

set(nowrap_headers
  vtkAffineArray.h
  vtkAffineImplicitBackend.h
  vtkCompositeArray.h
  vtkConstantArray.h
  vtkConstantImplicitBackend.h
  vtkImplicitArrayTraits.h
  vtkIndexedArray.h
  vtkStdFunctionArray.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkVTK_DISPATCH_IMPLICIT_ARRAYS.h"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkArrayDispatchImplicitArrayList.h"
)

set(nowrap_template_classes
  vtkImplicitArray
  vtkCompositeImplicitBackend
  vtkIndexedImplicitBackend
)

set(sources
  ${instantiation_sources}
)

vtk_module_add_module(VTK::CommonImplicitArrays
  NOWRAP_HEADERS ${nowrap_headers}
  NOWRAP_TEMPLATE_CLASSES ${nowrap_template_classes}
  SOURCES ${sources}
)

vtk_add_test_mangling(VTK::CommonImplicitArrays)

vtk_module_compile_features(VTK::CommonImplicitArrays
  PUBLIC
    cxx_std_11)
