## Improve vtkBlockIdScalars

vtkBlockIdScalars now support any vtkDataObjectTree, which include vtkPartitionedDataSetCollection
It also support iterating into a composite hierarchy thanks to new properties:
 - TraverseSubTree
 - VisitOnlyLeaves

 The default behavior has not changed.

 In that context, vtkBlockIdScalars now inherit vtkPassInputTypeAlgorithm and
 the protected method vtkBlockIdScalars::ColorBlock has been removed.
