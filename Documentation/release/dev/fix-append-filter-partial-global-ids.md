# vtkAppendFilter no longer crashes when input has partial a GlobalIds field

`vtkAppendFilter` no longer crashes if only some input datasets have global point IDs.
Global IDs are now used for point merging only when they are available on every input.
