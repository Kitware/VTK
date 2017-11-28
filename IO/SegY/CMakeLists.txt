set(Module_SRCS
  vtkSegY2DReader.cxx
  vtkSegY3DReader.cxx
  )

set (Module_PRIVATE_SRCS
  vtkSegYIOUtils.cxx
  vtkSegYReader.cxx
  vtkSegYTraceReader.cxx
  )

vtk_module_library(vtkIOSegY ${Module_SRCS} ${Module_PRIVATE_SRCS})
