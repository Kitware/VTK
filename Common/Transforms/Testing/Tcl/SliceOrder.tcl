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
  si SetMatrix 1 0 0 0  0 0 1 0  0 -1 0 0  0 0 0 1

vtkTransform is
  is SetMatrix 1 0 0 0  0 0 -1 0  0 -1 0 0  0 0 0 1

vtkTransform ap
  ap Scale 1 -1 1

vtkTransform pa
  pa Scale 1 -1 -1

vtkTransform lr
  lr SetMatrix 0 0 -1 0  0 -1 0 0  1 0 0 0  0 0 0 1

vtkTransform rl
  rl SetMatrix 0 0 1 0  0 -1 0 0  1 0 0 0  0 0 0 1

#
# the previous transforms assume radiological views of the slices (viewed from the feet). other
# modalities such as physical sectioning may view from the head. these transforms modify the original
# with a 180 rotation about y
#
vtkTransform hf
  hf SetMatrix -1 0 0 0  0 1 0 0  0 0 -1 0  0 0 0 1

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

