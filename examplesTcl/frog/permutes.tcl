
#
# these transformations permute medical image data to maintain proper orientation
# regardless os the acqusition order. After apply these transforms with
# vtkTransformFilter, a view up of 0,-1,0 will result in the body part
# facing the viewer. 
# NOTE: some transformations have a -1 scale factor for one of the components.
#       To ensure proper polygon orientation and normal direction, you must
#       apply the vtkPolyNormals filter.

vtkTransform is;
#	is RotateX 90;
[is GetMatrix] SetElement 0 0 1
[is GetMatrix] SetElement 0 1 0
[is GetMatrix] SetElement 0 2 0
[is GetMatrix] SetElement 0 3 0
[is GetMatrix] SetElement 1 0 0
[is GetMatrix] SetElement 1 1 0
[is GetMatrix] SetElement 1 2 -1
[is GetMatrix] SetElement 1 3 0
[is GetMatrix] SetElement 2 0 0
[is GetMatrix] SetElement 2 1 1
[is GetMatrix] SetElement 2 2 0
[is GetMatrix] SetElement 2 3 0
[is GetMatrix] SetElement 3 0 0
[is GetMatrix] SetElement 3 1 0
[is GetMatrix] SetElement 3 2 0
[is GetMatrix] SetElement 3 3 1

vtkTransform si;
#	si RotateX -90;
#	si Scale 1 -1 1;
[si GetMatrix] SetElement 0 0 1
[si GetMatrix] SetElement 0 1 0
[si GetMatrix] SetElement 0 2 0
[si GetMatrix] SetElement 0 3 0
[si GetMatrix] SetElement 1 0 0
[si GetMatrix] SetElement 1 1 0
[si GetMatrix] SetElement 1 2 1
[si GetMatrix] SetElement 1 3 0
[si GetMatrix] SetElement 2 0 0
[si GetMatrix] SetElement 2 1 1
[si GetMatrix] SetElement 2 2 0
[si GetMatrix] SetElement 2 3 0
[si GetMatrix] SetElement 3 0 0
[si GetMatrix] SetElement 3 1 0
[si GetMatrix] SetElement 3 2 0
[si GetMatrix] SetElement 3 3 1

vtkTransform pa;
	pa Scale 1 1 -1;

vtkTransform ap;

vtkTransform rl;
#	rl RotateY -90;
#	rl Scale 1 1 -1;
[rl GetMatrix] SetElement 0 0 0
[rl GetMatrix] SetElement 0 1 0
[rl GetMatrix] SetElement 0 2 1
[rl GetMatrix] SetElement 0 3 0
[rl GetMatrix] SetElement 1 0 0
[rl GetMatrix] SetElement 1 1 1
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

vtkTransform lr;
#	lr RotateY -90;
[lr GetMatrix] SetElement 0 0 0
[lr GetMatrix] SetElement 0 1 0
[lr GetMatrix] SetElement 0 2 -1
[lr GetMatrix] SetElement 0 3 0
[lr GetMatrix] SetElement 1 0 0
[lr GetMatrix] SetElement 1 1 1
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
