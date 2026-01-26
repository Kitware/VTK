## Fixed compile errors for IsHipDevicePointer

This function was trying to reference a member named `TYPE_ATTR` in the
`hipPointerAttribute_t`. This member does not exist.

The intention of `TYPE_ATTR` was probably a macro that selected the name
`memoryType` for HIP 5 and earlier and `type` for HIP 6 and later. Since
Viskores currently only supports HIP 6 and up, the code now just uses `type`.

The device pointer check also now includes `hipMemoryTypeManaged`. The
documentation is not clear what the difference between this and
`hipMemoryTypeUnified` is. Presumably it comes from these identifiers being a
mashup of identifiers from multiple CUDA enums. (See
https://rocm.docs.amd.com/projects/HIP/en/docs-6.0.0/doxygen/html/group___global_defs.html#gaea86e91d3cd65992d787b39b218435a3)
