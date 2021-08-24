# Adding a new selection type `BLOCK_SELECTORS`

Adds support to vtkSelectionNode for a new content type `BLOCK_SELECTORS`.
This type enables applications to define a selection for blocks in a composite dataset
using selector expression for hierarchy or in case of vtkPartitionedDataSetCollection, any
`vtkDataAssembly` associated with it.
