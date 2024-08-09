## Name targets after library names

VTK module targets are now named after their library name (e.g.,
`vtkCommonCore` rather than `CommonCore`). This should avoid conflicts with
targets provided by dependencies conflicting with VTK's vendored projects.
