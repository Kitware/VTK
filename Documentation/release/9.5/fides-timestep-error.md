# Remove error for missing Fides timestep

Previously, the Fides reader would issue an error if the pipeline requested
a timestep that could not be found in the reported time values. Since the
closest timevalue was always selected, this only really happened when the
Fides file did not specify timesteps.

However, it is not an error to request a time value from a reader that does
not have time. This is typical of ParaView, which always sets a consistent
time value for all the pipelines that it executes. Thus, ParaView would
report an error for any data file that did not set a time array.

This is fixed by setting an appropriate time index (0) for the case when
there are no time values to compare to.
