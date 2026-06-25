## Add vtkPTemporalPathLineFilter for partitioned dataset support

`vtkPTemporalPathLineFilter` extends `vtkTemporalPathLineFilter` with support for
`vtkPartitionedDataSet` input and parallel selection allgather.

When the input is a `vtkPartitionedDataSet`, the filter iterates over all partitions
and feeds them into the same trail map keyed by global particle ID. This enables
correct particle path line generation across partitioned temporal data, such as
output from `vtkParticleTracer` in distributed settings.

In parallel (MPI), selection IDs are allgathered so that all ranks produce
consistent output. When the input is a plain `vtkDataSet`, behavior is identical
to the superclass.
