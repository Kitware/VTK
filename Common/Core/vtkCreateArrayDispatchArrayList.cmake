# This file contains macros that are used by VTK to generate the list of
# default arrays used by the vtkArrayDispatch system.
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
# At a lower level, specific arrays can be added to the list individually in
# two ways:
#
# For templated classes, set the following variables:
# - vtkArrayDispatch_containers:
#   List of template class names.
# - vtkArrayDispatch_[template class name]_types:
#   For the specified template class, add an entry to the array list that
#   instantiates the container for each type listed here.
# - vtkArrayDispatch_[template class name]_header
#   Specifies the header file to include for the specified template class.
#
# Both templated and non-templated arrays can be added using these variables:
# - vtkArrayDispatch_extra_arrays:
#   List of arrays to add to the list.
# - vtkArrayDispatch_extra_headers:
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
#   vtkTypeList_Create_21(
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
#   )
# >::Result Arrays;
#
# } // end namespace vtkArrayDispatch
#
# #endif // vtkArrayDispatchArrayList_h
#

# Populate the environment so that vtk_array_dispatch_generate_array_header will
# create the array TypeList with all known array types.
macro(vtkArrayDispatch_default_array_setup)

# The default set of scalar types:
set(vtkArrayDispatch_all_types
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

# For each container, define a header and a list of types:
if (VTK_DISPATCH_AOS_ARRAYS)
  list(APPEND vtkArrayDispatch_containers vtkAOSDataArrayTemplate)
  set(vtkArrayDispatch_vtkAOSDataArrayTemplate_header vtkAOSDataArrayTemplate.h)
  set(vtkArrayDispatch_vtkAOSDataArrayTemplate_types
    ${vtkArrayDispatch_all_types}
  )
endif()

if (VTK_DISPATCH_SOA_ARRAYS)
  list(APPEND vtkArrayDispatch_containers vtkSOADataArrayTemplate)
  set(vtkArrayDispatch_vtkSOADataArrayTemplate_header vtkSOADataArrayTemplate.h)
  set(vtkArrayDispatch_vtkSOADataArrayTemplate_types
    ${vtkArrayDispatch_all_types}
  )
endif()

if (VTK_DISPATCH_TYPED_ARRAYS)
  list(APPEND vtkArrayDispatch_containers vtkTypedDataArray)
  set(vtkArrayDispatch_vtkTypedDataArray_header vtkTypedDataArray.h)
  set(vtkArrayDispatch_vtkTypedDataArray_types
    ${vtkArrayDispatch_all_types}
  )
endif()

endmacro()

# Concatenates a list of strings into a single string, since string(CONCAT ...)
# is not currently available for VTK's cmake version.
# Internal method.
function(CollapseString input output)
  set(temp "")
  foreach(line ${input})
    set(temp ${temp}${line})
  endforeach()
  set(${output} "${temp}" PARENT_SCOPE)
endfunction()

# Create a header that declares the vtkArrayDispatch::Arrays TypeList.
macro(vtkArrayDispatch_generate_array_header result)

set(vtkAD_headers vtkTypeList.h)
set(vtkAD_arrays)
foreach(container ${vtkArrayDispatch_containers})
  list(APPEND vtkAD_headers ${vtkArrayDispatch_${container}_header})
  foreach(value_type ${vtkArrayDispatch_${container}_types})
    list(APPEND vtkAD_arrays "${container}<${value_type}>")
  endforeach()
endforeach()

# Include externally specified headers/arrays:
list(APPEND vtkAD_headers ${vtkArrayDispatch_extra_headers})
list(APPEND vtkAD_arrays ${vtkArrayDispatch_extra_arrays})

set(temp
  "// This file is autogenerated by vtkCreateArrayDispatchArrayList.cmake.\n"
  "// Do not edit this file. Your changes will not be saved.\n"
  "\n"
  "#ifndef vtkArrayDispatchArrayList_h\n"
  "#define vtkArrayDispatchArrayList_h\n"
  "\n"
)

foreach(header ${vtkAD_headers})
  list(APPEND temp "#include \"${header}\"\n")
endforeach()

list(LENGTH vtkAD_arrays vtkAD_numArrays)

list(APPEND temp
  "\n"
  "namespace vtkArrayDispatch {\n"
  "\n"
  "typedef vtkTypeList::Unique<\n"
  "  vtkTypeList_Create_${vtkAD_numArrays}(\n"
)

foreach(array ${vtkAD_arrays})
  list(APPEND temp "    ${array},\n")
endforeach()

# Remove the final comma from the array list:
CollapseString("${temp}" temp)
string(REGEX REPLACE ",\n$" "\n" temp "${temp}")

list(APPEND temp
  "  )\n"
  ">::Result Arrays\;\n"
  "\n"
  "} // end namespace vtkArrayDispatch\n"
  "\n"
  "#endif // vtkArrayDispatchArrayList_h\n"
)

CollapseString("${temp}" ${result})

endmacro()
