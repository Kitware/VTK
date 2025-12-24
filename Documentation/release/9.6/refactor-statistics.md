## Statistics algorithms refactored

`vtkStatisticsAlgorithm` and its subclasses have been refactored to produce
instances of a new data object (`vtkStatisticalModel`) for models rather than
a `vtkMultiBlockDataSet` instance holding `vtkTable`s. This is a breaking
change as it would be very hard to reliably validate and test filters that
could be configured to produce different output types (especially since models
are used as both filter inputs and outputs).

This commit adds methods which enable any algorithm to
own an instance of `vtkStatisticsAlgorithmPrivate` (which
holds sets of requested attribute-array names) and copy its
contents to a `vtkStatisticsAlgorithm` subclass they own.
This way, a "wrapper" algorithm that adapts a `vtkDataObject`
into a `vtkTable` can run on *any* statistics algorithm
(instead of requiring a subclass of the wrapper for each
subclass of vtkStatisticsAlgorithm as was done in the past).

We also add `vtkGenerateStatistics` which adapts geometric and composite
VTK data objects into tables of samples and creates statistical
models of the samples. It accepts composite data held in a
distributed-memory environment and performs communication as
needed to build one model (or one hierarchy of models for composite
data if requested) across all ranks. It does not currently handle
multiblock datasets (you should convert to a partitioned dataset
collection instead) or cell grids (though there is some work in
this direction).

Because `vtkGenerateStatistics` owns and can apply an instance of any
subclass of `vtkStatisticsAlgorithm` to the tables it produces, the
`vtkStatisticsAlgorithm` class now implements methods to
serialize and deserialize ivars of any subclass. This allows
a `vtkStatisticalModel` (which stores the serialization) to
produce an instance of a properly-configured `vtkStatisticsAlgorithm`
to perform further processing (such as assessing data or testing the
likelihood of a model).

In order to facilitate communication of statistical model data
across ranks, both an XML and a legacy reader/writer pair for
the new vtkStatisticalModel data object are added.
The XML reader/writer only perform inline ASCII reads/writes for now.
Models are small, so that is all that should be required.

We add a new statistics algorithm for combined order+moment stats
called `vtkVisualStatistics`. It inherits `vtkDescriptiveStatistics`
and adds a fixed-width bin approximation to a histogram of data.
This has the potential to be faster than `vtkOrderStatistics` but
cannot capture unique frequent values the way order statistics can.

A new filter, `vtkSumTables` is provided to sum numeric columns
of a set of input `vtkTable` instances. This is used by
`vtkVisualStatistics` to aggregate binned histogram models.

`vtkExtractStatisticalModelTables` is a new filter to extract
model tables from a statistical model so that applications which
do not deal with `vtkStatisticalModel` data objects can still
access model information.

We centralize and expand ghost counting for statistics algorithms:
+ Move GhostsToSkip bit-mask and NumberOfGhosts ivar to the
  base `vtkStatisticsAlgorithm` class. This removes a lot of
  duplication in subclasses. Really, all subclasses should
  support skipping ghosts.
+ Expand support for skipping ghosted samples to the
  `vtkCorrelativeStatistics` filter.

As part of this change, `vtkPCAStatistics` has renamed its Test output
column from "Block" to "Partition," so if you have code that processes
the output of `vtkPCAStatistics` you may need to look for the column under
its new name (in addition to processing models in their new format).

Finally, this adds a new cell-grid query for creating sample tables
named `vtkCellGridSampleQuery`, but does not provide any responders.
