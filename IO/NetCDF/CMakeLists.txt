set(Module_SRCS
  vtkMPASReader.cxx
  vtkNetCDFCAMReader.cxx
  vtkNetCDFCFReader.cxx
  vtkNetCDFPOPReader.cxx
  vtkNetCDFReader.cxx
  vtkSLACParticleReader.cxx
  vtkSLACReader.cxx
  )

vtk_module_library(vtkIONetCDF ${Module_SRCS})

target_link_libraries(vtkIONetCDF LINK_PRIVATE vtkNetCDF_cxx)
