#!/usr/bin/env python
import numpy as np
import math

from vtkmodules.vtkCommonCore import \
        vtkDoubleArray, vtkIntArray, vtkUnsignedShortArray, \
        vtkUnsignedIntArray, vtkShortArray, vtkIdTypeArray, vtkStringArray
from vtkmodules.vtkCommonDataModel import \
        vtkDataObjectCollection, vtkStatisticalModel, vtkTable
from vtkmodules.vtkFiltersStatistics import \
        vtkStatisticsAlgorithm, vtkDescriptiveStatistics, vtkContingencyStatistics, \
        vtkCorrelativeStatistics, vtkMultiCorrelativeStatistics, vtkOrderStatistics, \
        vtkPCAStatistics, vtkVisualStatistics, vtkHighestDensityRegionsStatistics, \
        vtkAutoCorrelativeStatistics, vtkKMeansStatistics
from vtkmodules.vtkFiltersGeneral import vtkSumTables

from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def ParametersSurviveRoundTrip(algorithm):
    """Return true if a round-trip of the algorithms parameters reproduces them."""
    params = algorithm.GetAlgorithmParameters()
    print(params)
    dupe = vtkStatisticsAlgorithm.NewFromAlgorithmParameters(params)
    if params == dupe.GetAlgorithmParameters():
        return True
    print(f'ERROR: {params} vs {dupe.GetAlgorithmParameters()}')
    return False

def TestAlgorithmParameters():
    """Test that statistics algorithm subclasses properly serialize and deserialize
    their parameters so that vtkGenerateStatistics can reproduce the algorithm instance."""

    algorithmsWithNoParameters=(
        vtkAutoCorrelativeStatistics,
        vtkContingencyStatistics,
        vtkCorrelativeStatistics,
    )

    # Descriptive statistics
    descriptiveStats = vtkDescriptiveStatistics(sample_estimate=0,signed_deviations=1)
    assert(ParametersSurviveRoundTrip(descriptiveStats))
    descriptiveStats = vtkDescriptiveStatistics(sample_estimate=1,signed_deviations=0)
    assert(ParametersSurviveRoundTrip(descriptiveStats))
    descriptiveStats = vtkDescriptiveStatistics()
    assert(ParametersSurviveRoundTrip(descriptiveStats))

    # Highest-density regions statistics
    hdrStats = vtkHighestDensityRegionsStatistics(sigma=2.25)
    assert(ParametersSurviveRoundTrip(hdrStats))
    hdrStats = vtkHighestDensityRegionsStatistics(sigma_matrix=(1,2,2,3))
    assert(ParametersSurviveRoundTrip(hdrStats))

    # K-means statistics
    kmStats = vtkKMeansStatistics(default_number_of_clusters=9,k_values_array_name='kool',
                                  max_num_iterations=12, tolerance=1e-6)
    assert(ParametersSurviveRoundTrip(kmStats))

    # Multi-correlative statistics
    mcStats = vtkMultiCorrelativeStatistics(median_absolute_deviation=1)
    assert(ParametersSurviveRoundTrip(mcStats))
    mcStats = vtkMultiCorrelativeStatistics()
    assert(ParametersSurviveRoundTrip(mcStats))

    # Order statistics
    ordStats = vtkOrderStatistics(number_of_intervals=23, quantile_definition=0,
                                  quantize=1, maximum_histogram_size=1024)
    assert(ParametersSurviveRoundTrip(ordStats))
    ordStats = vtkOrderStatistics()
    assert(ParametersSurviveRoundTrip(ordStats))

    # Visual statistics
    vizStats = vtkVisualStatistics(number_of_bins=37)
    for field, rangeVal in {'foo':(-1,1),'bar':(0,1),'baz':(0.5,10.25)}.items():
        vizStats.SetFieldRange(field, rangeVal[0], rangeVal[1])
    assert(ParametersSurviveRoundTrip(vizStats))
    vizStats = vtkVisualStatistics()
    assert(ParametersSurviveRoundTrip(vizStats))

    # Algorithms that have no parameters should still work (and inherit the base
    # class parameters):
    for alg in algorithmsWithNoParameters:
        aa = alg()
        assert(ParametersSurviveRoundTrip(aa))

    return True

ok = TestAlgorithmParameters()
assert(ok)
