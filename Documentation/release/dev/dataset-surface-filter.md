## vtkDataSetSurfaceFilter: triangle strip support removed

vtkDataSetSurfaceFilter no longer supports generation of triangle strips.
Developts can use vtkStripper to convert the generate polydata to triangle
strips, if required. Triangle strips have long stopped being a way to get
rendering performance improvements. Consequently, the use-case for
vtkDataSetSurfaceFilter to generate triagle strips is no longer relevant.
Hence, this capability was removed from the filter for improved maintainability.
