#
# these transformations permute medical image data to maintain proper orientation
# regardless of the acqusition order. After applying these transforms with
# vtkTransformFilter, a view up of 0,-1,0 will result in the body part
# facing the viewer. 
# NOTE: some transformations have a -1 scale factor for one of the components.
#       To ensure proper polygon orientation and normal direction, you must
#       apply the vtkPolyDataNormals filter.
#
# Naming:
# si - superior to inferior (top to bottom)
# is - inferior to superior (bottom to top)
# ap - anterior to posterior (front to back)
# pa - posterior to anterior (back to front)
# lr - left to right
# rl - right to left
#
vtkTransform si
[si GetMatrixPointer] SetElement 0 0 1
[si GetMatrixPointer] SetElement 0 1 0
[si GetMatrixPointer] SetElement 0 2 0
[si GetMatrixPointer] SetElement 0 3 0
[si GetMatrixPointer] SetElement 1 0 0
[si GetMatrixPointer] SetElement 1 1 0
[si GetMatrixPointer] SetElement 1 2 1
[si GetMatrixPointer] SetElement 1 3 0
[si GetMatrixPointer] SetElement 2 0 0
[si GetMatrixPointer] SetElement 2 1 -1
[si GetMatrixPointer] SetElement 2 2 0
[si GetMatrixPointer] SetElement 2 3 0
[si GetMatrixPointer] SetElement 3 0 0
[si GetMatrixPointer] SetElement 3 1 0
[si GetMatrixPointer] SetElement 3 2 0
[si GetMatrixPointer] SetElement 3 3 1

vtkTransform is
[is GetMatrixPointer] SetElement 0 0 1
[is GetMatrixPointer] SetElement 0 1 0
[is GetMatrixPointer] SetElement 0 2 0
[is GetMatrixPointer] SetElement 0 3 0
[is GetMatrixPointer] SetElement 1 0 0
[is GetMatrixPointer] SetElement 1 1 0
[is GetMatrixPointer] SetElement 1 2 -1
[is GetMatrixPointer] SetElement 1 3 0
[is GetMatrixPointer] SetElement 2 0 0
[is GetMatrixPointer] SetElement 2 1 -1
[is GetMatrixPointer] SetElement 2 2 0
[is GetMatrixPointer] SetElement 2 3 0
[is GetMatrixPointer] SetElement 3 0 0
[is GetMatrixPointer] SetElement 3 1 0
[is GetMatrixPointer] SetElement 3 2 0
[is GetMatrixPointer] SetElement 3 3 1

vtkTransform ap
	ap Scale 1 -1 1

vtkTransform pa
	pa Scale 1 -1 -1

vtkTransform lr
[lr GetMatrixPointer] SetElement 0 0 0
[lr GetMatrixPointer] SetElement 0 1 0
[lr GetMatrixPointer] SetElement 0 2 -1
[lr GetMatrixPointer] SetElement 0 3 0
[lr GetMatrixPointer] SetElement 1 0 0
[lr GetMatrixPointer] SetElement 1 1 -1
[lr GetMatrixPointer] SetElement 1 2 0
[lr GetMatrixPointer] SetElement 1 3 0
[lr GetMatrixPointer] SetElement 2 0 1
[lr GetMatrixPointer] SetElement 2 1 0
[lr GetMatrixPointer] SetElement 2 2 0
[lr GetMatrixPointer] SetElement 2 3 0
[lr GetMatrixPointer] SetElement 3 0 0
[lr GetMatrixPointer] SetElement 3 1 0
[lr GetMatrixPointer] SetElement 3 2 0
[lr GetMatrixPointer] SetElement 3 3 1

vtkTransform rl
[rl GetMatrixPointer] SetElement 0 0 0
[rl GetMatrixPointer] SetElement 0 1 0
[rl GetMatrixPointer] SetElement 0 2 1
[rl GetMatrixPointer] SetElement 0 3 0
[rl GetMatrixPointer] SetElement 1 0 0
[rl GetMatrixPointer] SetElement 1 1 -1
[rl GetMatrixPointer] SetElement 1 2 0
[rl GetMatrixPointer] SetElement 1 3 0
[rl GetMatrixPointer] SetElement 2 0 1
[rl GetMatrixPointer] SetElement 2 1 0
[rl GetMatrixPointer] SetElement 2 2 0
[rl GetMatrixPointer] SetElement 2 3 0
[rl GetMatrixPointer] SetElement 3 0 0
[rl GetMatrixPointer] SetElement 3 1 0
[rl GetMatrixPointer] SetElement 3 2 0
[rl GetMatrixPointer] SetElement 3 3 1

#
# the previous transforms assume radiological views of the slices (viewed from the feet). other
# modalities such as physical sectioning may view from the head. these transforms modify the original
# with a 180 rotation about y
#
vtkTransform hf
[hf GetMatrixPointer] SetElement 0 0 -1
[hf GetMatrixPointer] SetElement 0 1 0
[hf GetMatrixPointer] SetElement 0 2 0
[hf GetMatrixPointer] SetElement 0 3 0
[hf GetMatrixPointer] SetElement 1 0 0
[hf GetMatrixPointer] SetElement 1 1 1
[hf GetMatrixPointer] SetElement 1 2 0
[hf GetMatrixPointer] SetElement 1 3 0
[hf GetMatrixPointer] SetElement 2 0 0
[hf GetMatrixPointer] SetElement 2 1 0
[hf GetMatrixPointer] SetElement 2 2 -1
[hf GetMatrixPointer] SetElement 2 3 0
[hf GetMatrixPointer] SetElement 3 0 0
[hf GetMatrixPointer] SetElement 3 1 0
[hf GetMatrixPointer] SetElement 3 2 0
[hf GetMatrixPointer] SetElement 3 3 1

vtkTransform hfsi
  hfsi Concatenate [hf GetMatrixPointer]
  hfsi Concatenate [si GetMatrixPointer]

vtkTransform hfis
  hfis Concatenate [hf GetMatrixPointer]
  hfis Concatenate [is GetMatrixPointer]

vtkTransform hfap
  hfap Concatenate [hf GetMatrixPointer]
  hfap Concatenate [ap GetMatrixPointer]

vtkTransform hfpa
  hfpa Concatenate [hf GetMatrixPointer]
  hfpa Concatenate [pa GetMatrixPointer]

vtkTransform hflr
  hflr Concatenate [hf GetMatrixPointer]
  hflr Concatenate [lr GetMatrixPointer]

vtkTransform hfrl
  hfrl Concatenate [hf GetMatrixPointer]
  hfrl Concatenate [rl GetMatrixPointer]

