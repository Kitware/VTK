## vtkGroupDataSetsFilter: a filter to create composite datasets

vtkGroupDataSetsFilter is a new filter that can be used to combine input
datasets into a  vtkMultBlockDataSet, vtkPartitionedDataSet, or a
vtkPartitionedDataSetCollection. The inputs are added a individual blocks in
the output and can be named assigned block-names using `SetInputName`.

This is a more generic version of `vtkMultiBlockDataGroupFilter` and should
be preferred.
