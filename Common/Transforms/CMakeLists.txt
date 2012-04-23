set(Module_SRCS
  vtkAbstractTransform.cxx
  vtkCylindricalTransform.cxx
  vtkGeneralTransform.cxx
  vtkHomogeneousTransform.cxx
  vtkIdentityTransform.cxx
  vtkLinearTransform.cxx
  vtkMatrixToHomogeneousTransform.cxx
  vtkMatrixToLinearTransform.cxx
  vtkPerspectiveTransform.cxx
  vtkSphericalTransform.cxx
  vtkThinPlateSplineTransform.cxx
  vtkTransform2D.cxx
  vtkTransformCollection.cxx
  vtkTransform.cxx
  vtkWarpTransform.cxx
  vtkLandmarkTransform.cxx
  )

set_source_files_properties(
  vtkAbstractTransform
  vtkHomogeneousTransform
  vtkLinearTransform
  vtkWarpTransform
  ABSTRACT
  )

vtk_module_library(vtkCommonTransforms ${Module_SRCS})
