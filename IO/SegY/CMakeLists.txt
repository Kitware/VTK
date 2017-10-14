set(Module_SRCS
  vtkSegY2DReader.cxx
  vtkSegY3DReader.cxx
  )

set (Module_PRIVATE_SRCS
  vtkSegYIOUtils.cxx
  vtkSegYReader.cxx
  vtkSegYTraceReader.cxx
  )

set_source_files_properties(
  ${Module_PRIVATE_SRCS}
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkIOSegY ${Module_SRCS} ${Module_PRIVATE_SRCS})
