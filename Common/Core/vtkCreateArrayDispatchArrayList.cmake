# This file contains macros that are used by VTK to generate the list of
# default and implicit arrays used by the vtkArrayDispatch system.
#
# There are a number of CMake variables that control the final array list. At
# the high level, the following options enable/disable predefined categories of
# arrays:
#
# - VTK_DISPATCH_AOS_ARRAYS (default: ON)
#   Include vtkAOSDataArrayTemplate<ValueType> for the basic types supported
#   by VTK. This should probably not be turned off.
# - VTK_DISPATCH_SOA_ARRAYS (default: OFF)
#   Include vtkSOADataArrayTemplate<ValueType> for the basic types supported
#   by VTK.
#
# - VTK_DISPATCH_AFFINE_ARRAYS (default: OFF)
#   Include vtkAffineArray<ValueType> for the basic types supported
#   by VTK.
# - VTK_DISPATCH_CONSTANT_ARRAYS (default: OFF)
#   Include vtkConstantArray<ValueType> for the basic types supported
#   by VTK.
# - VTK_DISPATCH_STD_FUNCTION_ARRAYS (default: OFF)
#   Include vtkStdFunctionArray<ValueType> for the basic types supported
#   by VTK.
# - VTK_DISPATCH_STRIDED_ARRAYS (default: OFF)
#   Include vtkStridedArray<ValueType> for the basic types supported
#   by VTK.
# - VTK_DISPATCH_STRUCTURED_POINT_ARRAYS (default: ON)
#   Include vtkStructuredPointArray<ValueType> for the basic types supported
#   by VTK. This should probably not be turned off.
#
# At a lower level, specific arrays can be added to the list individually in
# two ways:
#
# For templated classes, set the following variables:
# - vtkArrayDispatch_containers:
# - vtkArrayDispatchImplicit_containers:
#   List of template class names.
# - vtkArrayDispatch_[template class name]_types:
# - vtkArrayDispatchImplicit_[template class name]_types:
#   For the specified template class, add an entry to the array list that
#   instantiates the container for each type listed here.
# - vtkArrayDispatch_[template class name]_header
# - vtkArrayDispatchImplicit_[template class name]_header
#   Specifies the header file to include for the specified template class.
#
# Both templated and non-templated arrays can be added using these variables:
# - vtkArrayDispatch_extra_arrays:
# - vtkArrayDispatchImplicit_extra_arrays:
#   List of arrays to add to the list.
# - vtkArrayDispatch_extra_headers:
# - vtkArrayDispatchImplicit_extra_headers:
#   List of headers to include.
#
################################ Example #######################################
#
# The cmake call below instantiates the array list that follows:
#
# cmake [path to VTK source]
#   -DvtkArrayDispatch_containers="MyCustomArray1;MyCustomArray2"
#   -DvtkArrayDispatch_MyCustomArray1_header="MyCustomArray1.h"
#   -DvtkArrayDispatch_MyCustomArray1_types="float;double"
#   -DvtkArrayDispatch_MyCustomArray2_header="MyCustomArray2.h"
#   -DvtkArrayDispatch_MyCustomArray2_types="int;unsigned char"
#   -DvtkArrayDispatch_extra_headers="ExtraHeader1.h;ExtraHeader2.h"
#   -DvtkArrayDispatch_extra_arrays="ExtraArray1;ExtraArray2<float>;ExtraArray2<char>"
#   -DvtkArrayDispatchImplicit_containers="MyCustomArray1;MyCustomArray2"
#   -DvtkArrayDispatchImplicit_MyCustomArray1_header="MyCustomArray1.h"
#   -DvtkArrayDispatchImplicit_MyCustomArray1_types="float;double"
#   -DvtkArrayDispatchImplicit_MyCustomArray2_header="MyCustomArray2.h"
#   -DvtkArrayDispatchImplicit_MyCustomArray2_types="int;unsigned char"
#   -DvtkArrayDispatchImplicit_extra_headers="ExtraHeader1.h;ExtraHeader2.h"
#   -DvtkArrayDispatchImplicit_extra_arrays="ExtraArray1;ExtraArray2<float>;ExtraArray2<char>"
#
# Generated header:
#
# #ifndef vtkArrayDispatchArrayList_h
# #define vtkArrayDispatchArrayList_h
#
# #include "vtkTypeList.h"
# #include "MyCustomArray1.h"
# #include "MyCustomArray2.h"
# #include "vtkAOSDataArrayTemplate.h"
# #include "ExtraHeader1.h"
# #include "ExtraHeader2.h"
#
# namespace vtkArrayDispatch {
#
# typedef vtkTypeList::Unique<
#   vtkTypeList::Create<
#     MyCustomArray1<float>,
#     MyCustomArray1<double>,
#     MyCustomArray2<int>,
#     MyCustomArray2<unsigned char>,
#     vtkAOSDataArrayTemplate<char>,
#     vtkAOSDataArrayTemplate<double>,
#     vtkAOSDataArrayTemplate<float>,
#     vtkAOSDataArrayTemplate<int>,
#     vtkAOSDataArrayTemplate<long>,
#     vtkAOSDataArrayTemplate<short>,
#     vtkAOSDataArrayTemplate<signed char>,
#     vtkAOSDataArrayTemplate<unsigned char>,
#     vtkAOSDataArrayTemplate<unsigned int>,
#     vtkAOSDataArrayTemplate<unsigned long>,
#     vtkAOSDataArrayTemplate<unsigned short>,
#     vtkAOSDataArrayTemplate<vtkIdType>,
#     vtkAOSDataArrayTemplate<long long>,
#     vtkAOSDataArrayTemplate<unsigned long long>,
#     ExtraArray1,
#     ExtraArray2<float>,
#     ExtraArray2<char>
#   >
# >::Result Arrays;
#
# typedef vtkTypeList::Unique<
#   vtkTypeListvtkTypeList::Create<
#     MyCustomArray1<float>,
#     MyCustomArray1<double>,
#     MyCustomArray2<int>,
#     MyCustomArray2<unsigned char>,
#     ExtraArray1,
#     ExtraArray2<float>,
#     ExtraArray2<char>
#   >
# >::Result ReadOnlyArrays;
#
# typedef vtkTypeList::Unique< vtkTypeList::TypeList<Arrays, ReadOnlyArrays> >::Result AllArrays;
#
# VTK_ABI_NAMESPACE_END
#
# } // end namespace vtkArrayDispatch
#
# #endif // vtkArrayDispatchArrayList_h
#

# get vtk_numeric_types
include(vtkTypeLists)

# Populate the environment so that vtk_array_dispatch_generate_array_header will
# create the array TypeList with all known array types.
macro(vtkArrayDispatch_default_array_setup)

  # Helper macro to create array dispatch entries
  macro(_vtkCreateArrayDispatch var class types)
    if (${var})
      list(APPEND vtkArrayDispatch_containers "${class}")
      set("vtkArrayDispatch_${class}_header" "${class}.h")
      set("vtkArrayDispatch_${class}_types" "${types}")
    endif ()
  endmacro()

  # Set up regular arrays
  _vtkCreateArrayDispatch(VTK_DISPATCH_AOS_ARRAYS "vtkAOSDataArrayTemplate" "${vtk_numeric_types}")
  _vtkCreateArrayDispatch(VTK_DISPATCH_SOA_ARRAYS "vtkSOADataArrayTemplate" "${vtk_numeric_types}")

  # Helper macro for implicit arrays
  macro(_vtkCreateArrayDispatchImplicit var class types)
    if (${var})
      list(APPEND vtkArrayDispatchImplicit_containers "${class}")
      set("vtkArrayDispatchImplicit_${class}_header" "${class}.h")
      set("vtkArrayDispatchImplicit_${class}_types" "${types}")
    endif ()
  endmacro()

  # Set up implicit arrays
  _vtkCreateArrayDispatchImplicit(VTK_DISPATCH_AFFINE_ARRAYS "vtkAffineArray" "${vtk_numeric_types}")
  _vtkCreateArrayDispatchImplicit(VTK_DISPATCH_CONSTANT_ARRAYS "vtkConstantArray" "${vtk_numeric_types}")
  _vtkCreateArrayDispatchImplicit(VTK_DISPATCH_STD_FUNCTION_ARRAYS "vtkStdFunctionArray" "${vtk_numeric_types}")
  _vtkCreateArrayDispatchImplicit(VTK_DISPATCH_STRIDED_ARRAYS "vtkStridedArray" "${vtk_numeric_types}")
  _vtkCreateArrayDispatchImplicit(VTK_DISPATCH_STRUCTURED_POINT_ARRAYS "vtkStructuredPointArray" "${vtk_numeric_types}")

endmacro()

# Create a header that declares the vtkArrayDispatch::Arrays TypeList.
macro(vtkArrayDispatch_generate_array_header result)

  # Initialize
  set(vtkAD_headers "vtkTypeList.h")

  # Create separate lists for each regular array type
  set(vtkAD_aos_arrays)
  set(vtkAD_soa_arrays)
  set(vtkAD_extra_arrays)
  # Process regular arrays
  foreach (container IN LISTS vtkArrayDispatch_containers)
    list(APPEND vtkAD_headers "${vtkArrayDispatch_${container}_header}")
    foreach (value_type IN LISTS "vtkArrayDispatch_${container}_types")
      if (container STREQUAL "vtkAOSDataArrayTemplate")
        list(APPEND vtkAD_aos_arrays "${container}<${value_type}>")
      elseif (container STREQUAL "vtkSOADataArrayTemplate")
        list(APPEND vtkAD_soa_arrays "${container}<${value_type}>")
      else ()
        list(APPEND vtkAD_extra_arrays "${container}<${value_type}>")
      endif ()
    endforeach ()
  endforeach ()

  # Create separate lists for each implicit array type
  set(vtkAD_affine_arrays)
  set(vtkAD_constant_arrays)
  set(vtkAD_std_function_arrays)
  set(vtkAD_strided_arrays)
  set(vtkAD_structured_point_arrays)
  set(vtkAD_implicit_extra_arrays)
  # Process implicit arrays
  foreach (container IN LISTS vtkArrayDispatchImplicit_containers)
    list(APPEND vtkAD_headers "${vtkArrayDispatchImplicit_${container}_header}")
    foreach (value_type IN LISTS "vtkArrayDispatchImplicit_${container}_types")
      if (container STREQUAL "vtkAffineArray")
        list(APPEND vtkAD_affine_arrays "${container}<${value_type}>")
      elseif (container STREQUAL "vtkConstantArray")
        list(APPEND vtkAD_constant_arrays "${container}<${value_type}>")
      elseif (container STREQUAL "vtkStdFunctionArray")
        list(APPEND vtkAD_std_function_arrays "${container}<${value_type}>")
      elseif (container STREQUAL "vtkStridedArray")
        list(APPEND vtkAD_strided_arrays "${container}<${value_type}>")
      elseif (container STREQUAL "vtkStructuredPointArray")
        list(APPEND vtkAD_structured_point_arrays "${container}<${value_type}>")
      else ()
        list(APPEND vtkAD_implicit_extra_arrays "${container}<${value_type}>")
      endif ()
    endforeach ()
  endforeach ()

  # Include externally specified headers/arrays:
  list(APPEND vtkAD_headers ${vtkArrayDispatch_extra_headers})
  list(APPEND vtkAD_extra_arrays ${vtkArrayDispatch_extra_arrays})
  list(APPEND vtkAD_headers ${vtkArrayDispatchImplicit_extra_headers})
  list(APPEND vtkAD_implicit_extra_arrays ${vtkArrayDispatchImplicit_extra_arrays})

  # Start building the header content
  set(temp
    "// This file is autogenerated by vtkCreateArrayDispatchImplicitList.cmake.\n"
    "// Do not edit this file. Your changes will not be saved.\n"
    "\n"
    "#ifndef vtkArrayDispatchArrayList_h\n"
    "#define vtkArrayDispatchArrayList_h\n"
    "\n"
  )

  # Add includes
  foreach (header IN LISTS vtkAD_headers)
    list(APPEND temp "#include \"${header}\"\n")
  endforeach ()

  list(APPEND temp
    "\n"
    "namespace vtkArrayDispatch {\n"
    "VTK_ABI_NAMESPACE_BEGIN\n"
    "\n"
  )

  # Helper macro to generate typedef for an array category
  macro(_vtkGenerateTypeList list_name type_name)
    list(APPEND temp
      "using ${type_name} = vtkTypeList::Unique<vtkTypeList::Create<"
    )
    set(vtkAD_sep "")
    foreach (array IN LISTS ${list_name})
      list(APPEND temp "${vtkAD_sep}\n    ${array}")
      set(vtkAD_sep ",")
    endforeach ()
    list(APPEND temp
      "\n>>::Result\;\n\n"
    )
  endmacro()

  # Generate individual array type lists
  _vtkGenerateTypeList(vtkAD_aos_arrays "AOSArrays")
  _vtkGenerateTypeList(vtkAD_soa_arrays "SOAArrays")
  _vtkGenerateTypeList(vtkAD_extra_arrays "ExtraArrays")

  # Combine all mutable arrays
  list(APPEND temp
    "using Arrays = vtkTypeList::Append<\n"
    "  AOSArrays,\n"
    "  SOAArrays,\n"
    "  ExtraArrays\n"
    ">::Result\;\n\n"
  )

  _vtkGenerateTypeList(vtkAD_affine_arrays "AffineArrays")
  _vtkGenerateTypeList(vtkAD_constant_arrays "ConstantArrays")
  _vtkGenerateTypeList(vtkAD_std_function_arrays "StdFunctionArrays")
  _vtkGenerateTypeList(vtkAD_strided_arrays "StridedArrays")
  _vtkGenerateTypeList(vtkAD_structured_point_arrays "StructuredPointArrays")
  _vtkGenerateTypeList(vtkAD_implicit_extra_arrays "ImplicitExtraArrays")

  # Combine all read-only arrays
  list(APPEND temp
    "using ReadOnlyArrays = vtkTypeList::Append<\n"
    "  AffineArrays,\n"
    "  ConstantArrays,\n"
    "  StdFunctionArrays,\n"
    "  StridedArrays,\n"
    "  StructuredPointArrays,\n"
    "  ImplicitExtraArrays\n"
    ">::Result\;\n\n"
  )

  # Combine all arrays
  list(APPEND temp
    "using AllArrays = vtkTypeList::Append<Arrays, ReadOnlyArrays>::Result\;\n"
    "\n"
    "VTK_ABI_NAMESPACE_END\n"
    "\n"
    "} // end namespace vtkArrayDispatch\n"
    "#endif // vtkArrayDispatchArrayList_h\n"
  )

  string(CONCAT ${result} ${temp})
endmacro()
