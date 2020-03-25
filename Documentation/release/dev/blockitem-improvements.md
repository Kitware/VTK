## Improved text rendering support in `vtkBlockItem`

`vtkBlockItem` is a `vtkContextItem` that can be used to add
a labelled block in chart views. `vtkBlockItem` now supports resizing
itself based on the label specified. Furthermore, you can use new API
added to the `vtkBlockItem` to change rendering parameters such as brush,
pen, text properties for the label, padding, and margins.
