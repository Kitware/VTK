# This file contains macros that are used by VTK to generate the list of
# implicit arrays used by the vtkArrayDispatch system.
#
# There are a number of CMake variables that control the final array list. At
# the high level, the following options enable/disable predefined categories of
# arrays:
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
#
# At a lower level, specific arrays can be added to the list individually in
# two ways:
#
# For templated classes, set the following variables:
# - vtkArrayDispatchImplicit_containers:
#   List of template class names.
# - vtkArrayDispatchImplicit_[template class name]_types:
#   For the specified template class, add an entry to the array list that
#   instantiates the container for each type listed here.
# - vtkArrayDispatchImplicit_[template class name]_header
#   Specifies the header file to include for the specified template class.
#
# Both templated and non-templated arrays can be added using these variables:
# - vtkArrayDispatchImplicit_extra_arrays:
#   List of arrays to add to the list.
# - vtkArrayDispatchImplicit_extra_headers:
#   List of headers to include.
#
################################ Example #######################################
#
# The cmake call below instantiates the array list that follows:
#
# cmake [path to VTK source]
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
# #ifndef vtkArrayDispatchImplicitArrayList_h
# #define vtkArrayDispatchImplicitArrayList_h
#
# #include "vtkTypeList.h"
# #include "MyCustomArray1.h"
# #include "MyCustomArray2.h"
# #include "ExtraHeader1.h"
# #include "ExtraHeader2.h"
# #include "vtkArrayDispatchArrayList.h"
#
# namespace vtkArrayDispatch {
#
# VTK_ABI_NAMESPACE_BEGIN
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
# #endif // vtkArrayDispatchImplicitArrayList_h
#

# Populate the environment so that vtk_array_dispatch_generate_array_header will
# create the array TypeList with all known array types.
macro(vtkArrayDispatchImplicit_default_array_setup)

# The default set of scalar types:
set(vtkArrayDispatchImplicit_all_types
  "char"
  "double"
  "float"
  "int"
  "long"
  "long long"
  "short"
  "signed char"
  "unsigned char"
  "unsigned int"
  "unsigned long"
  "unsigned long long"
  "unsigned short"
  "vtkIdType"
)

if (VTK_DISPATCH_STD_FUNCTION_ARRAYS)
  list(APPEND vtkArrayDispatchImplicit_containers vtkStdFunctionArray)
  set(vtkArrayDispatchImplicit_vtkStdFunctionArray_header vtkStdFunctionArray.h)
  set(vtkArrayDispatchImplicit_vtkStdFunctionArray_types
    ${vtkArrayDispatchImplicit_all_types}
  )
endif()

if (VTK_DISPATCH_CONSTANT_ARRAYS)
  list(APPEND vtkArrayDispatchImplicit_containers vtkConstantArray)
  set(vtkArrayDispatchImplicit_vtkConstantArray_header vtkConstantArray.h)
  set(vtkArrayDispatchImplicit_vtkConstantArray_types
    ${vtkArrayDispatchImplicit_all_types}
  )
endif()

if (VTK_DISPATCH_AFFINE_ARRAYS)
  list(APPEND vtkArrayDispatchImplicit_containers vtkAffineArray)
  set(vtkArrayDispatchImplicit_vtkAffineArray_header vtkAffineArray.h)
  set(vtkArrayDispatchImplicit_vtkAffineArray_types
    ${vtkArrayDispatchImplicit_all_types}
  )
endif()

if (VTK_DISPATCH_COMPOSITE_ARRAYS)
  list(APPEND vtkArrayDispatchImplicit_containers vtkCompositeArray)
  set(vtkArrayDispatchImplicit_vtkCompositeArray_header vtkCompositeArray.h)
  set(vtkArrayDispatchImplicit_vtkCompositeArray_types
    ${vtkArrayDispatchImplicit_all_types}
  )
endif()

if (VTK_DISPATCH_INDEXED_ARRAYS)
  list(APPEND vtkArrayDispatchImplicit_containers vtkIndexedArray)
  set(vtkArrayDispatchImplicit_vtkIndexedArray_header vtkIndexedArray.h)
  set(vtkArrayDispatchImplicit_vtkIndexedArray_types
    ${vtkArrayDispatchImplicit_all_types}
  )
endif()

endmacro()

# Create a header that declares the vtkArrayDispatch::Arrays TypeList.
macro(vtkArrayDispatchImplicit_generate_array_header result)

set(vtkAD_headers vtkTypeList.h)
list(APPEND vtkAD_headers vtkArrayDispatchArrayList.h)
set(vtkAD_arrays)
foreach(container ${vtkArrayDispatchImplicit_containers})
  list(APPEND vtkAD_headers ${vtkArrayDispatchImplicit_${container}_header})
  foreach(value_type ${vtkArrayDispatchImplicit_${container}_types})
    list(APPEND vtkAD_arrays "${container}<${value_type}>")
  endforeach()
endforeach()

# Include externally specified headers/arrays:
list(APPEND vtkAD_headers ${vtkArrayDispatchImplicit_extra_headers})
list(APPEND vtkAD_arrays ${vtkArrayDispatchImplicit_extra_arrays})

set(temp
  "// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen\n"
  "// SPDX-License-Identifier: BSD-3-Clause\n"
  "// This file is autogenerated by vtkCreateArrayDispatchImplicitList.cmake.\n"
  "// Do not edit this file. Your changes will not be saved.\n"
  "// Funded by CEA, DAM, DIF, F-91297 Arpajon, France\n"
  "\n"
  "#ifndef vtkArrayDispatchImplicitArrayList_h\n"
  "#define vtkArrayDispatchImplicitArrayList_h\n"
  "\n"
)

foreach(header ${vtkAD_headers})
  list(APPEND temp "#include \"${header}\"\n")
endforeach()

list(APPEND temp
  "\n"
  "namespace vtkArrayDispatch {\n"
  "VTK_ABI_NAMESPACE_BEGIN\n"
  "\n"
  "typedef vtkTypeList::Unique<\n"
  "  vtkTypeList::Create<\n"
)

foreach(array ${vtkAD_arrays})
  list(APPEND temp "    ${array},\n")
endforeach()

# Remove the final comma from the array list:
string(CONCAT temp ${temp})
string(REGEX REPLACE ",\n$" "\n" temp "${temp}")

list(APPEND temp
  "  >\n"
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
  "#endif // vtkArrayDispatchImplicitArrayList_h\n"
)

string(CONCAT ${result} ${temp})

endmacro()
