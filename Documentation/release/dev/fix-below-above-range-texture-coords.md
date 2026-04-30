## Handle floating-point error when using Below Range/Above Range colors in vtkMapper

Datasets with point or cell data arrays and values at the Below Range or Above Range colors could be color mapped wrong in some cases due to floating-point errors when mapping data values to texture coordinates. That has been fixed by explicitily checking the computed texture coordinates and adjusting them by epsilon so they are on the same side of the texture range boundary as the data range boundary.
