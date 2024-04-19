# This file generates vtkImplicitArrays subclass for concrete type,
# like `vtkConstanteDoubleArray` or `vtkIdTypeCompositeArray`.
#
# Generated classes are not templated thus they can be wrapped.

# Configure `.in` class files depending on the requested backend
# and the concrete c++ type.
macro(_generate_implicit_array_specialization backend vtk_type concrete_type)
  # used inside .in files
  set(VTK_TYPE_NAME "${vtk_type}")
  set(CONCRETE_TYPE "${concrete_type}")

  set(className "vtk${backend}${VTK_TYPE_NAME}Array")

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${backend}TypedArray.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${className}.h"
    @ONLY)

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/vtk${backend}TypedArray.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${className}.cxx"
    @ONLY)

  # append generated files to current module headers and sources
  list(APPEND headers
    "${CMAKE_CURRENT_BINARY_DIR}/${className}.h")
  list(APPEND sources
    "${CMAKE_CURRENT_BINARY_DIR}/${className}.cxx")
endmacro()

foreach(backend IN ITEMS Constant Affine Indexed Composite)
  _generate_implicit_array_specialization("${backend}" Char char)
  _generate_implicit_array_specialization("${backend}" Double double)
  _generate_implicit_array_specialization("${backend}" Float float)
  _generate_implicit_array_specialization("${backend}" IdType vtkIdType)
  _generate_implicit_array_specialization("${backend}" Int int)
  _generate_implicit_array_specialization("${backend}" Long long)
  _generate_implicit_array_specialization("${backend}" Short short)
  _generate_implicit_array_specialization("${backend}" SignedChar "signed char")
  _generate_implicit_array_specialization("${backend}" UnsignedChar "unsigned char")
  _generate_implicit_array_specialization("${backend}" UnsignedInt "unsigned int")
  _generate_implicit_array_specialization("${backend}" UnsignedLong "unsigned long")
  _generate_implicit_array_specialization("${backend}" UnsignedShort "unsigned short")
endforeach()
