## Rework vtkCompositeDataSet::ShallowCopy

- vtkCompositeDataSet::ShallowCopy now do an actual shallow copy up to array pointers
- Introduce a vtkCompositeDataSet::CompositeShallowCopy that shallow copy up to dataset pointers only
- Deprecate vtkCompositeDataSet::RecursiveShallowCopy
