# This file generates arrays specialization subclasses for fixed types,
# like `vtkConstantTypeFloat32Array` or `vtkAffineTypeInt64Array`.
#
# Generated classes are not templated thus they can be wrapped.

# Configure `.in` class files depending on the requested backend
# and the concrete c++ type.
macro(_generate_array_specialization array_prefix vtk_type concrete_type deprecated)
  # used inside .in files
  set(VTK_TYPE_NAME "${vtk_type}")
  set(CONCRETE_TYPE "${concrete_type}")
  if ("${deprecated}")
    set(VTK_DEPRECATION "VTK_DEPRECATED_IN_9_6_0(\"Use vtk${array_prefix}Type*Array instead\")")
  else ()
    set(VTK_DEPRECATION "")
  endif ()

  set(_className "vtk${array_prefix}${VTK_TYPE_NAME}Array")

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${array_prefix}TypedArray.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.h"
    @ONLY)

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${array_prefix}TypedArray.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.cxx"
    @ONLY)

  # append generated header to current module headers
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/${_className}.h")

  # append generated source to the bulk instantiation of concrete_type
  string(REPLACE " " "_" _suffix "${concrete_type}")
  list(APPEND "bulk_instantiation_sources_${_suffix}"
    "#include \"${_className}.cxx\"")

  unset(VTK_DEPRECATION)
  unset(VTK_TYPE_NAME)
  unset(CONCRETE_TYPE)
  unset(_className)
endmacro()

include(vtkTypeLists)

# VTK_DEPRECATED_IN_9_6_0 to be removed later
foreach (array_prefix IN ITEMS Affine Composite Constant Indexed)
  foreach (type IN LISTS vtk_numeric_types)
    vtk_type_to_camel_case("${type}" cased_type)
    _generate_array_specialization("${array_prefix}" "${cased_type}" "${type}" 1)
  endforeach ()
endforeach ()

foreach (array_prefix IN ITEMS Affine Composite Constant Indexed ScaledSOA SOA StdFunction Strided)
  foreach (type IN LISTS vtk_fixed_size_numeric_types)
    vtk_fixed_size_type_to_without_prefix("${type}" "vtk" without_vtk_prefix)
    _generate_array_specialization("${array_prefix}" "${without_vtk_prefix}" "${type}" 0)
  endforeach ()
endforeach ()

function(vtk_type_native type ctype class)
  string(TOUPPER "${type}" type_upper)
  set("vtk_type_native_${type}" "
#if VTK_TYPE_${type_upper} == VTK_${ctype}
# include \"${class}Array.h\"
# define vtkTypeArrayBase ${class}Array
#endif
"
    PARENT_SCOPE)
endfunction()

function(vtk_type_native_fallback type preferred_ctype preferred_class fallback_class)
  string(TOUPPER "${type}" type_upper)
  set("vtk_type_native_${type}" "
#if VTK_TYPE_${type_upper} == VTK_${preferred_ctype}
# include \"${preferred_class}Array.h\"
# define vtkTypeArrayBase ${preferred_class}Array
#else
# include \"${fallback_class}Array.h\"
# define vtkTypeArrayBase ${fallback_class}Array
#endif
"
    PARENT_SCOPE)
endfunction()

function(vtk_type_native_choice type preferred_ctype preferred_class fallback_ctype fallback_class)
  string(TOUPPER "${type}" type_upper)
  set("vtk_type_native_${type}" "
#if VTK_TYPE_${type_upper} == VTK_${preferred_ctype}
# include \"${preferred_class}Array.h\"
# define vtkTypeArrayBase ${preferred_class}Array
#elif VTK_TYPE_${type_upper} == VTK_${fallback_ctype}
# include \"${fallback_class}Array.h\"
# define vtkTypeArrayBase ${fallback_class}Array
#endif
"
    PARENT_SCOPE)
endfunction()

# Configure data arrays for platform-independent fixed-size types.
# Match the type selection here to that in vtkType.h.
vtk_type_native_fallback(Int8 CHAR vtkChar vtkSignedChar)
vtk_type_native(UInt8 UNSIGNED_CHAR vtkUnsignedChar)
vtk_type_native(Int16 SHORT vtkShort)
vtk_type_native(UInt16 UNSIGNED_SHORT vtkUnsignedShort)
vtk_type_native(Int32 INT vtkInt)
vtk_type_native(UInt32 UNSIGNED_INT vtkUnsignedInt)
vtk_type_native_choice(Int64 LONG vtkLong LONG_LONG vtkLongLong)
vtk_type_native_choice(UInt64 UNSIGNED_LONG vtkUnsignedLong UNSIGNED_LONG_LONG vtkUnsignedLongLong)
vtk_type_native(Float32 FLOAT vtkFloat)
vtk_type_native(Float64 DOUBLE vtkDouble)

foreach (type IN LISTS vtk_fixed_size_numeric_types)
  vtk_fixed_size_type_to_without_prefix("${type}" "vtkType" vtk_type)
  set(VTK_TYPE_NAME "${vtk_type}")
  set(VTK_TYPE_NATIVE "${vtk_type_native_${vtk_type}}")
  if (VTK_TYPE_NATIVE)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkAOSTypedArray.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${type}Array.h"
      @ONLY)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkAOSTypedArray.cxx.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${type}Array.cxx"
      @ONLY)
    # append generated header to current module headers
    list(APPEND headers
      "${CMAKE_CURRENT_BINARY_DIR}/${type}Array.h")
    # append generated source to the bulk instantiation of concrete_type
    string(REPLACE " " "_" _suffix "${type}")
    list(APPEND "bulk_instantiation_sources_${_suffix}"
      "#include \"${type}Array.cxx\"")
  endif ()
endforeach ()
