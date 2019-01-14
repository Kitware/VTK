set(classes
  vtkAbstractTransform
  vtkCylindricalTransform
  vtkGeneralTransform
  vtkHomogeneousTransform
  vtkIdentityTransform
  vtkLandmarkTransform
  vtkLinearTransform
  vtkMatrixToHomogeneousTransform
  vtkMatrixToLinearTransform
  vtkPerspectiveTransform
  vtkSphericalTransform
  vtkThinPlateSplineTransform
  vtkTransform
  vtkTransform2D
  vtkTransformCollection
  vtkWarpTransform)

vtk_module_add_module(VTK::CommonTransforms
  CLASSES ${classes})
