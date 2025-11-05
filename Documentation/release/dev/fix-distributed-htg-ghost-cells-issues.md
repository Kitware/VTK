## Fix distributed htg ghost cells issues

- Ghost array is now correctly added to distributed HTG when ghost cells are requested by the executive.
- HTG metadatas are now exchanged between processes in the htg ghost cell generators when running the filter in a distributed context (fix paraview/paraview#22776)

`TreeGhostArray` and `TreeGhostArrayCached` protected members of `vtkHyperTreeGrid` have been moved to private.
