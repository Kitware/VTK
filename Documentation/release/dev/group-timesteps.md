## vtkGroupTimeStepsFilter: filter to group timesteps

`vtkGroupTimeStepsFilter` is a new filter that replaces
`vtkMultiBlockFromTimeSeriesFilter`. It accepts a temporal input and converts
that into either a vtkPartitionedDataSetCollection
or a vtkMultiBlockDataSet with all the timesteps.

vtkPartitionedDataSetCollection is preferred type for the output
unless the input is vtkMultiBlockDataSet, in which case this filter
produces vtkMultiBlockDataSet.
