# Fix cell-grid initialization

Static debug builds on Windows could crash at initialization as
the Schwarz counter to prepare the singleton container used to
register cell-grid cell metadata subclasses was not incremented
in time. See [#19552](https://gitlab.kitware.com/vtk/vtk/-/issues/19552)
for more information.
