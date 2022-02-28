## Selection Improvements

`vtkSelection` now supports the xor boolean operator.

`vtkSelectionSource` can now 1) generate multiple selection nodes, 2) has the option to define the field option using
either FieldType(vtkSelectionNode::SelectionField) or ElementType(vtkDataObject::AttributeTypes) and 3) defines the
ProcessId of the selection.

`vtkAppendSelection` can now append multiple selections by setting an expression using selection input names.

`vtkFrustumSelection` has been optimized to perform the minimum number of new operations needed.

`vtkExtractSelectedThresholds`, `vtkExtractSelectedPolyDataIds`, `vtkExtractSelectedLocations`, `vtkExtractSelectedIds`,
and `vtkExtractSelectedBlock` have been deprecated since `vtkExtractSelection` can be used instead of them.

`vtkConvertSelection` can now convert selection nodes with content type `BLOCK`/`BLOCK_SELECTORS` to `INDICES`.

`vtkHierarchicalBoxDataIterator` has been deprecated, and `vtkUniformGridAMRDataIterator` should be used instead.

Fix selection extraction for `vtkUniformGridAMR`-based datasets.
