set(Module_SRCS
  vtkTextureMapToCylinder.cxx
  vtkTextureMapToPlane.cxx
  vtkTextureMapToSphere.cxx
  vtkImplicitTextureCoords.cxx
  vtkThresholdTextureCoords.cxx
  vtkTransformTextureCoords.cxx
  vtkTriangularTCoords.cxx
  )

vtk_module_library(vtkFiltersTexture ${Module_SRCS})
