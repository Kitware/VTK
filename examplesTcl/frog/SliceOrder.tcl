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
[si GetMatrix] SetElement 0 0 1
[si GetMatrix] SetElement 0 1 0
[si GetMatrix] SetElement 0 2 0
[si GetMatrix] SetElement 0 3 0
[si GetMatrix] SetElement 1 0 0
[si GetMatrix] SetElement 1 1 0
[si GetMatrix] SetElement 1 2 1
[si GetMatrix] SetElement 1 3 0
[si GetMatrix] SetElement 2 0 0
[si GetMatrix] SetElement 2 1 -1
[si GetMatrix] SetElement 2 2 0
[si GetMatrix] SetElement 2 3 0
[si GetMatrix] SetElement 3 0 0
[si GetMatrix] SetElement 3 1 0
[si GetMatrix] SetElement 3 2 0
[si GetMatrix] SetElement 3 3 1

vtkTransform is
[is GetMatrix] SetElement 0 0 1
[is GetMatrix] SetElement 0 1 0
[is GetMatrix] SetElement 0 2 0
[is GetMatrix] SetElement 0 3 0
[is GetMatrix] SetElement 1 0 0
[is GetMatrix] SetElement 1 1 0
[is GetMatrix] SetElement 1 2 -1
[is GetMatrix] SetElement 1 3 0
[is GetMatrix] SetElement 2 0 0
[is GetMatrix] SetElement 2 1 -1
[is GetMatrix] SetElement 2 2 0
[is GetMatrix] SetElement 2 3 0
[is GetMatrix] SetElement 3 0 0
[is GetMatrix] SetElement 3 1 0
[is GetMatrix] SetElement 3 2 0
[is GetMatrix] SetElement 3 3 1

vtkTransform ap
	ap Scale 1 -1 1

vtkTransform pa
	pa Scale 1 -1 -1

vtkTransform lr
[lr GetMatrix] SetElement 0 0 0
[lr GetMatrix] SetElement 0 1 0
[lr GetMatrix] SetElement 0 2 -1
[lr GetMatrix] SetElement 0 3 0
[lr GetMatrix] SetElement 1 0 0
[lr GetMatrix] SetElement 1 1 -1
[lr GetMatrix] SetElement 1 2 0
[lr GetMatrix] SetElement 1 3 0
[lr GetMatrix] SetElement 2 0 1
[lr GetMatrix] SetElement 2 1 0
[lr GetMatrix] SetElement 2 2 0
[lr GetMatrix] SetElement 2 3 0
[lr GetMatrix] SetElement 3 0 0
[lr GetMatrix] SetElement 3 1 0
[lr GetMatrix] SetElement 3 2 0
[lr GetMatrix] SetElement 3 3 1

vtkTransform rl
[rl GetMatrix] SetElement 0 0 0
[rl GetMatrix] SetElement 0 1 0
[rl GetMatrix] SetElement 0 2 1
[rl GetMatrix] SetElement 0 3 0
[rl GetMatrix] SetElement 1 0 0
[rl GetMatrix] SetElement 1 1 -1
[rl GetMatrix] SetElement 1 2 0
[rl GetMatrix] SetElement 1 3 0
[rl GetMatrix] SetElement 2 0 1
[rl GetMatrix] SetElement 2 1 0
[rl GetMatrix] SetElement 2 2 0
[rl GetMatrix] SetElement 2 3 0
[rl GetMatrix] SetElement 3 0 0
[rl GetMatrix] SetElement 3 1 0
[rl GetMatrix] SetElement 3 2 0
[rl GetMatrix] SetElement 3 3 1

#
# the previous transforms assume radiological views of the slices (viewed from the feet). other
# modalities such as physical sectioning may view from the head. these transforms modify the original
# with a 180 rotation about y
#
vtkTransform hf
[hf GetMatrix] SetElement 0 0 -1
[hf GetMatrix] SetElement 0 1 0
[hf GetMatrix] SetElement 0 2 0
[hf GetMatrix] SetElement 0 3 0
[hf GetMatrix] SetElement 1 0 0
[hf GetMatrix] SetElement 1 1 1
[hf GetMatrix] SetElement 1 2 0
[hf GetMatrix] SetElement 1 3 0
[hf GetMatrix] SetElement 2 0 0
[hf GetMatrix] SetElement 2 1 0
[hf GetMatrix] SetElement 2 2 -1
[hf GetMatrix] SetElement 2 3 0
[hf GetMatrix] SetElement 3 0 0
[hf GetMatrix] SetElement 3 1 0
[hf GetMatrix] SetElement 3 2 0
[hf GetMatrix] SetElement 3 3 1

vtkTransform hfsi
  hfsi Concatenate [hf GetMatrix]
  hfsi Concatenate [si GetMatrix]

vtkTransform hfis
  hfis Concatenate [hf GetMatrix]
  hfis Concatenate [is GetMatrix]

vtkTransform hfap
  hfap Concatenate [hf GetMatrix]
  hfap Concatenate [ap GetMatrix]

vtkTransform hfpa
  hfpa Concatenate [hf GetMatrix]
  hfpa Concatenate [pa GetMatrix]

vtkTransform hflr
  hflr Concatenate [hf GetMatrix]
  hflr Concatenate [lr GetMatrix]

vtkTransform hfrl
  hfrl Concatenate [hf GetMatrix]
  hfrl Concatenate [rl GetMatrix]

