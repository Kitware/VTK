set(headers
  src/H5BlockErrors.h
  src/H5Block.h
  src/H5BlockPrivate.h
  src/H5BlockReadWrite.h
  src/H5BlockTypes.h
  src/H5MultiBlockErrors.h
  src/H5MultiBlock.h
  src/H5MultiBlockPrivate.h
  src/H5MultiBlockReadWrite.h
  src/H5MultiBlockTypes.h
  src/H5PartAttrib.h
  src/H5PartErrors.h
  src/H5PartF.h
  src/H5Part.h
  src/H5PartPrivate.h
  src/H5PartTypes.h
  src/Underscore.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkh5part_export.h")

set(sources
  src/H5Block.c
  src/H5BlockF.c
  src/H5BlockReadWrite.c
  src/H5BlockReadWriteF.c
  src/H5MultiBlock.c
  src/H5MultiBlockReadWrite.c
  src/H5PartAttrib.c
  src/H5PartAttribF.c
  src/H5Part.c
  src/H5PartF.c
  src/TestUnderscoreC.c)

vtk_module_add_module(VTK::h5part
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtkh5part")

include(GenerateExportHeader)
generate_export_header(h5part
  EXPORT_MACRO_NAME VTKH5PART_EXPORT
  EXPORT_FILE_NAME vtkh5part_export.h)
