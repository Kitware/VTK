function (vtk_type_native type ctype class)
  string(TOUPPER "${type}" type_upper)
  set("vtk_type_native_${type}" "
#if VTK_TYPE_${type_upper} == VTK_${ctype}
# include \"${class}Array.h\"
# define vtkTypeArrayBase ${class}Array
#endif
"
    PARENT_SCOPE)
endfunction ()

function (vtk_type_native_fallback type preferred_ctype preferred_class fallback_class)
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
endfunction ()

function (vtk_type_native_choice type preferred_ctype preferred_class fallback_ctype fallback_class)
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
endfunction ()

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

foreach(vtk_type IN ITEMS Int8 Int16 Int32 Int64 UInt8 UInt16 UInt32 UInt64 Float32 Float64)
  set(VTK_TYPE_NAME ${vtk_type})
  set(VTK_TYPE_NATIVE "${vtk_type_native_${vtk_type}}")
  if(VTK_TYPE_NATIVE)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/vtkType${vtk_type}Array.h"
      @ONLY)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/vtkTypedArray.cxx.in"
      "${CMAKE_CURRENT_BINARY_DIR}/vtkType${vtk_type}Array.cxx"
      @ONLY)
    list(APPEND sources
      "${CMAKE_CURRENT_BINARY_DIR}/vtkType${vtk_type}Array.cxx")
    list(APPEND headers
      "${CMAKE_CURRENT_BINARY_DIR}/vtkType${vtk_type}Array.h")
  endif()
endforeach()
