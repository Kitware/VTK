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

target_link_libraries(vtkIONetCDF vtkNetCDF_cxx)
set_target_properties(vtkIONetCDF PROPERTIES LINK_INTERFACE_LIBRARIES "")
