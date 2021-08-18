# Add vtkMergeTimeFilter

vtkMergeTimeFilter takes multiple temporal datasets as input and synchronize them.

The output data is a multiblock dataset containing one block per input dataset.
The output timesteps is the union (or the intersection) of each input timestep lists.
Duplicates time values are removed, dependending on a tolerance, either absolute or relative.
