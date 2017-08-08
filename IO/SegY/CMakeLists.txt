set(Module_SRCS
  vtkSegY2DReader.cxx
  vtkSegY3DReader.cxx
  vtkSegYIOUtils.cxx
  vtkSegYReader.cxx
  vtkSegYTraceReader.cxx
  )

vtk_module_library(vtkIOSegY ${Module_SRCS})
