## Fix ResampleWithDataSet with an HTG source using MPI

In previous versions, distributed HTG where incorrectly resampled, which could led to the apparition of undesired NaN values in the output. Now, the ResampleWithDataset filter works as expected for distributed HTG.
