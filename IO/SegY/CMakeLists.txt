set(Module_SRCS
  vtkSegYReader.cxx
  )

set (Module_PRIVATE_SRCS
  vtkSegYIOUtils.cxx
  vtkSegYReaderInternal.cxx
  vtkSegYTraceReader.cxx
  )

vtk_module_library(vtkIOSegY ${Module_SRCS} ${Module_PRIVATE_SRCS})
