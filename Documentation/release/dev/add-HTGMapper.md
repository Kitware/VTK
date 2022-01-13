# Add vtkHyperTreeGridMapper

A new HyperTree Grid mapper has been introduced for 2D HTG.
It is able to only render the part visible in the camera frustum.
In 2D, this mapper involve Camera parallel projection and works only when the camera
axis is orthogonal to the mapped HTG.
In 3D, this mapper fallback to the original one.
