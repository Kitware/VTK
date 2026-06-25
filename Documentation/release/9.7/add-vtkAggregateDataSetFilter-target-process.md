## vtkAggregateDataSetFilter: Add Target Process

The `vtkAggregateDataSetFilter` has a new `TargetProcess` property, which has the following options:

1. `vtkAggregateDataSetFilter::TargetProcessType::ROOT_PROCESS`: the data from all processes are aggregated to the
   root process of a (sub)controller. This is useful to ensure consistent aggregation across data that their number of
   points change over time.
2. `vtkAggregateDataSetFilter::TargetProcessType::PROCESS_WITH_MOST_POINTS` (default): the data from all processes are
   aggregated to the process that has the most points. This is useful to minimize the amount of data that needs to be
   sent across the network, but it can lead to inconsistent aggregation across time if the process with the most points
   changes over time.
