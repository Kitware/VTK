# Add vtkHyperTreeGridMapper

A new HyperTree Grid mapper has been introduced for 2D HTG (only composed of quads).
It is able to only render the part visible in the camera frustum.
This mapper involve Camera parallel projection and works only when the camera
axis is orthogonal to the mapped HTG.
