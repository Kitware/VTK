## Improved vtkCellGrid support in data model and filters

### `vtkDataObject::GetAttributeTypeAsString`

`vtkDataObject` now provides a new static method `GetAttributeTypeAsString(int attributeType)` that converts an integer
attribute-type constant (e.g., `vtkDataObject::POINT`, `vtkDataObject::CELL`) to its string name. This is the symmetric
counterpart of the existing `GetAssociationTypeAsString`.

### `vtkCellGrid::GetAttributesAsFieldData`

`vtkCellGrid` now overrides `GetAttributesAsFieldData(int type)` from `vtkDataObject`. Generic algorithms that call
`GetAttributesAsFieldData` on a `vtkDataObject` pointer now work correctly when the underlying dataset is a
`vtkCellGrid`.

### `vtkAttributeDataToTableFilter`: Support `vtkCellGrid`

`vtkAttributeDataToTableFilter` now accepts `vtkCellGrid` as input, in addition to the dataset and composite types it
already handled. This allows attribute data from a `vtkCellGrid` to be converted to a table.

### `vtkCellGridSummaryInformationQuery`: New query for per-attribute summary information

A new `vtkCellGridSummaryInformationQuery` computes per-attribute summary information for a `vtkCellGrid` in a single
traversal. For each `vtkCellAttribute`, the query accumulates:

* **Polynomial order range** — the minimum and maximum polynomial order across all cell types that carry the attribute.
  Retrieve it with `GetOrderRange(vtkCellAttribute*)`.
* **Degrees of freedom count** — the total number of degrees of freedom for the attribute, correctly accounting for
  shared DOFs when the attribute uses DOF sharing. Retrieve it with `GetNumberOfDOF(vtkCellAttribute*)`.

The accompanying `vtkDGSummaryInformationResponder` provides the responder implementation for discontinuous Galerkin (
DG) cell types.
