set(Module_SRCS
  vtkGeoJSONFeature.cxx
  vtkGeoJSONReader.cxx
  vtkGeoJSONWriter.cxx
  )

vtk_module_library(vtkIOGeoJSON ${Module_SRCS})
