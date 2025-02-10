## Generify and rename HTG filter VisibleLeavesSize to GenerateFields

Rename `vtkHyperTreeGridVisibleLeavesSize` to `vtkHyperTreeGridGenerateFields`.\
`vtkHyperTreeGridVisibleLeavesSize` is now deprecated.\
`vtkHyperTreeGridGenerateFields` is a more generic version of its predecessor, allowing easier addition of new fields.\
You can now add new fields by creating a class that inherits from `vtkHyperTreeGridGenerateField`, override the necessary methods, and then emplace it in the constructor of `vtkHyperTreeGridGenerateFields`.
