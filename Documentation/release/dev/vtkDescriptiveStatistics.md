## vtkDescriptiveStatistics changes

The class `vtkDescriptiveStatistics` has been reworked in 9.2. Kurtosis formula had a mistake that
is now corrected, and the API of this class has changed. Previously, one could turn on and off which
version of the kurtosis, skewness and variance was to be computed: the sample version or the population
version.
This was being done by toggling on and off `UnbiasedVariance` for the variance, `G1Skewness` for the
skewness, and `G2Kurtosis`. This switch is now centralized in the new `SampleEstimate` boolean.
When turned on, the sample version of each statistics is computed. When turned off, the population
version is instead. By default, the sample version is computed.

This change is introduced to simplify the interaction with the filter, and to avoid confusion from
the user, who used to be able to compute mixed types of statistics, when one shouldn't. Indeed, given an input
dataset, it makes no sense to compute the sample skewness while computing the population kurtosis,
as this choice should be made globally, depending on what kind of statistics is wanted. Both should
be either the sample version, or the population version.

The documentation is also enhanced, and the explicit formulas for each statistics in each case is
fully given.

The previous API dealing with the variance, skewness and kurtosis is ***disabled***. This means that
toggling `UnbiasedVariance`, `G1Skewness` and `G2Kurtosis` has no effect on the filter. Those APIs
are deprecated and will be removed at some point. A warning is displayed on usage of those methods.
One should toggle `SampleEstimate` instead.
