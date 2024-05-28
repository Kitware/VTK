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
# - VTK_DISPATCH_TYPED_ARRAYS (default: OFF)
#   Include vtkTypedDataArray<ValueType> for the basic types supported
#   by VTK. This enables the old-style in-situ vtkMappedDataArray subclasses
#   to be used.
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

macro(_vtkCreateArrayDispatch var class types)
  if (${var})
    list(APPEND vtkArrayDispatch_containers "${class}")
    set("vtkArrayDispatch_${class}_header" "${class}.h")
    set("vtkArrayDispatch_${class}_types" "${types}")
  endif ()
endmacro()

_vtkCreateArrayDispatch(VTK_DISPATCH_AOS_ARRAYS "vtkAOSDataArrayTemplate"
  "${vtk_numeric_types}")

_vtkCreateArrayDispatch(VTK_DISPATCH_SOA_ARRAYS "vtkSOADataArrayTemplate"
  "${vtk_numeric_types}")

if (VTK_DISPATCH_SOA_ARRAYS AND VTK_BUILD_SCALED_SOA_ARRAYS)
  set(_dispatch_scaled_soa_arrays "ON")
else ()
  set(_dispatch_scaled_soa_arrays "OFF")
endif ()
_vtkCreateArrayDispatch(_dispatch_scaled_soa_arrays "vtkScaledSOADataArrayTemplate"
  "${vtk_numeric_types}")

_vtkCreateArrayDispatch(VTK_DISPATCH_TYPED_ARRAYS "vtkTypedDataArray"
  "${vtk_numeric_types}")

macro(_vtkCreateArrayDispatchImplicit var class types)
  if (${var})
    list(APPEND vtkArrayDispatchImplicit_containers "${class}")
    set("vtkArrayDispatchImplicit_${class}_header" "${class}.h")
    set("vtkArrayDispatchImplicit_${class}_types" "${types}")
  endif ()
endmacro()

_vtkCreateArrayDispatchImplicit(VTK_DISPATCH_AFFINE_ARRAYS "vtkAffineArray"
  "${vtk_numeric_types}")

_vtkCreateArrayDispatchImplicit(VTK_DISPATCH_CONSTANT_ARRAYS "vtkConstantArray"
  "${vtk_numeric_types}")

_vtkCreateArrayDispatchImplicit(VTK_DISPATCH_STD_FUNCTION_ARRAYS "vtkStdFunctionArray"
  "${vtk_numeric_types}")

# we only need to dispatch on double for implicit point arrays
set(vtkArrayDispatchImplicit_structured_point_types "double")
_vtkCreateArrayDispatchImplicit(VTK_DISPATCH_STRUCTURED_POINT_ARRAYS "vtkStructuredPointArray"
  "${vtkArrayDispatchImplicit_structured_point_types}")

endmacro()

# Create a header that declares the vtkArrayDispatch::Arrays TypeList.
macro(vtkArrayDispatch_generate_array_header result)

set(vtkAD_headers vtkTypeList.h)
set(vtkAD_arrays)
set(vtkAD_readonly_arrays)
foreach(container IN LISTS vtkArrayDispatch_containers)
  list(APPEND vtkAD_headers "${vtkArrayDispatch_${container}_header}")
  foreach(value_type IN LISTS "vtkArrayDispatch_${container}_types")
    list(APPEND vtkAD_arrays "${container}<${value_type}>")
  endforeach()
endforeach ()
foreach(container IN LISTS vtkArrayDispatchImplicit_containers)
  list(APPEND vtkAD_headers "${vtkArrayDispatchImplicit_${container}_header}")
  foreach(value_type IN LISTS "vtkArrayDispatchImplicit_${container}_types")
    list(APPEND vtkAD_readonly_arrays "${container}<${value_type}>")
  endforeach()
endforeach()

# Include externally specified headers/arrays:
list(APPEND vtkAD_headers ${vtkArrayDispatch_extra_headers})
list(APPEND vtkAD_arrays ${vtkArrayDispatch_extra_arrays})

set(temp
  "// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen\n"
  "// SPDX-License-Identifier: BSD-3-Clause\n"
  "// This file is autogenerated by vtkCreateArrayDispatchImplicitList.cmake.\n"
  "// Do not edit this file. Your changes will not be saved.\n"
  "\n"
  "#ifndef vtkArrayDispatchArrayList_h\n"
  "#define vtkArrayDispatchArrayList_h\n"
  "\n"
)

foreach(header IN LISTS vtkAD_headers)
  list(APPEND temp "#include \"${header}\"\n")
endforeach()

list(APPEND temp
  "\n"
  "namespace vtkArrayDispatch {\n"
  "VTK_ABI_NAMESPACE_BEGIN\n"
  "\n"
  "typedef vtkTypeList::Unique<\n"
  "  vtkTypeList::Create<"
)

set(vtkAD_sep "")
foreach (array IN LISTS vtkAD_arrays)
  list(APPEND temp "${vtkAD_sep}\n    ${array}")
  set(vtkAD_sep ",")
endforeach ()

list(APPEND temp
  "\n  >\n"
  ">::Result Arrays\;\n"
  "\n"
  "typedef vtkTypeList::Unique<\n"
  "  vtkTypeList::Create<\n"
  )

set(vtkAD_sep "")
foreach (array IN LISTS vtkAD_readonly_arrays)
  list(APPEND temp "${vtkAD_sep}\n    ${array}")
  set(vtkAD_sep ",")
endforeach ()

list(APPEND temp
  "\n  >\n"
  ">::Result ReadOnlyArrays\;\n"
  "\n"
  "typedef vtkTypeList::Unique<\n"
  "vtkTypeList::Append<Arrays,\n"
  "ReadOnlyArrays>::Result\n"
  ">::Result AllArrays\;\n"
  "\n"
  "VTK_ABI_NAMESPACE_END\n"
  "\n"
  "} // end namespace vtkArrayDispatch\n"
  "#endif // vtkArrayDispatchArrayList_h\n"
)

string(CONCAT ${result} ${temp})

endmacro()
