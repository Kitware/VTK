#!/usr/bin/env python
import numpy as np
import math

from vtkmodules.vtkCommonCore import \
        vtkDoubleArray, vtkIntArray, vtkUnsignedShortArray, \
        vtkUnsignedIntArray, vtkShortArray, vtkIdTypeArray, vtkStringArray
from vtkmodules.vtkCommonDataModel import \
        vtkDataObjectCollection, vtkStatisticalModel, vtkTable
from vtkmodules.vtkFiltersStatistics import vtkVisualStatistics, vtkStatisticsAlgorithm
from vtkmodules.vtkFiltersGeneral import vtkSumTables

from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

def ArrayFromList(arrayType, name, vals):
    array = arrayType()
    array.SetName(name)
    array.SetNumberOfTuples(len(vals))
    ii = 0
    for val in vals:
        array.SetValue(ii, val)
        ii += 1
    return array

def CompareTables(result, expected):
    if result.GetNumberOfColumns() != expected.GetNumberOfColumns():
        print('  ERROR: Column count mismatch')
        return False
    status = True
    for cc in range(result.GetNumberOfColumns()):
        rc = result.GetColumn(cc)
        ec = expected.GetColumnByName(rc.GetName())
        if rc.IsNumeric() and ec.IsNumeric():
            nn = rc.GetNumberOfTuples()
            for ii in range(nn):
                rv = np.array(rc.GetTuple(ii))
                ev = np.array(ec.GetTuple(ii))
                if np.any(rv != ev):
                    srv = ' '.join([str(xx) for xx in rv])
                    sev = ' '.join([str(xx) for xx in ev])
                    print(f'  ERROR: {rc.GetName()} values mismatched at row {ii}: {srv}, expected {sev}')
                    status = False
        else:
            nn = rc.GetNumberOfValues()
            for ii in range(nn):
                rv = rc.GetValue(ii)
                ev = ec.GetValue(ii)
                if rv != ev:
                    print(f'  ERROR: {rc.GetName()} values mismatched at row {ii}: {rv}, expected {ev}')
                    status = False
    return status

def TestAggregation(algo, model, summaryTab, histoTab, expected):
    """Test that aggregating a statistical model with a copy of itself is computed properly."""
    ok = True
    aggModel = vtkStatisticalModel()
    collection = vtkDataObjectCollection()
    collection.AddItem(model)
    collection.AddItem(model)
    algo.Aggregate(collection, aggModel)
    nbins = algo.GetNumberOfBins()
    aggHisto = aggModel.GetTable(vtkStatisticalModel.Learned, 1)
    aggSummary = aggModel.GetTable(vtkStatisticalModel.Learned, 2)
    srcHisto = model.GetTable(vtkStatisticalModel.Learned, 1)
    srcSummary = model.GetTable(vtkStatisticalModel.Learned, 2)
    # Create new "expected" tables holding double the sample count
    # (which is the expected outcome for aggregating a sample with itself):
    expectedHisto = vtkTable()
    for entry in expected:
        expectedHisto.AddColumn(ArrayFromList(vtkIdTypeArray, entry['field'], 2 * np.array(entry['counts'])))
    if not CompareTables(aggHisto, expectedHisto):
        print('  ERROR: Aggregated histogram unexpected.')
        print('  Aggregated histogram')
        aggHisto.Dump(10,-1)
        ok = False
    expTotals = vtkIdTypeArray(name="Totals", number_of_tuples=len(expected))
    [expTotals.SetValue(ii, 2*np.sum(np.array(expected[ii]['counts'])[1:(nbins + 1)])) for ii in range(len(expected))]
    expNames = vtkStringArray(name="Name", number_of_tuples=len(expected))
    [expNames.SetValue(ii, expected[ii]['field']) for ii in range(len(expected))]
    expectedSummary = vtkTable()
    expectedSummary.AddColumn(expNames)
    expectedSummary.AddColumn(expTotals)
    if not CompareTables(aggSummary, expectedSummary):
        print('  ERROR: Aggregated summary unexpected.')
        print('Aggregated summary')
        aggSummary.Dump(10,-1)
        ok = False
    return ok

def VisualStatisticsTest(msg, arrays, ranges, expected):
    """Test that histograms over the given arrays and ranges match the expected counts."""
    print(f'Testing {msg}.')
    table1 = vtkTable()
    for array in arrays:
        table1.AddColumn(array)
    vstat = vtkVisualStatistics()
    vstat.SetInputDataObject(0, table1)
    for field, extent in ranges.items():
        vstat.AddColumn(field)
        vstat.SetFieldRange(field, extent[0], extent[1])
    nbins = len(expected[0]['counts']) - 3
    vstat.SetNumberOfBins(nbins)
    vstat.Update()
    model = vstat.GetOutputDataObject(vtkStatisticsAlgorithm.OUTPUT_MODEL)
    histoTab = model.GetTable(vtkStatisticalModel.Learned, 1)
    summaryTab = model.GetTable(vtkStatisticalModel.Learned, 2)
    print('  Summary table')
    summaryTab.Dump(10,-1)
    # Now test the results:
    ok = True
    expectedTab = vtkTable()
    for entry in expected:
        expectedTab.AddColumn(ArrayFromList(vtkIdTypeArray, entry['field'], entry['counts']))
    if not CompareTables(histoTab, expectedTab):
        barf = vtkTable()
        barf.DeepCopy(histoTab)
        for field, extent in ranges.items():
            bins = [xx*float(extent[1]-extent[0])/nbins + extent[0] for xx in range(nbins)]
            strbins = [f'<{extent[0]}', ] + [str(bb) for bb in bins] + [f'>{extent[1]}', 'NaN']
            barf.AddColumn(ArrayFromList(vtkStringArray, f'{field}_bin', strbins))
        print('  Input table')
        table1.Dump(10, -1)
        print('  Output table')
        barf.Dump(10, -1)
        print(f'  ERROR: Failed {msg} test due to incorrect learn results.')
        ok = False
    if not TestAggregation(vstat, model, summaryTab, histoTab, expected):
        print(f'  ERROR: Failed {msg} test due to incorrect aggregation.')
        ok = False
    return ok

ok = VisualStatisticsTest('histogram binning',
    (
        ArrayFromList(vtkDoubleArray, 'dbl', [-1.5, 0, 2.5, 4.25, 1.55, 0, 2.25]),
        ArrayFromList(vtkIntArray, 'int', [-1, 0, 3, 5, 3, 5, 21]),
        ArrayFromList(vtkUnsignedShortArray, 'ush', [1, 0, 3, 5, 5, 5, 4]),
    ),
    { 'dbl': [-7,7], 'int': [-2, 30], 'ush': [0, 5] },
    (
        { 'field': 'dbl', 'counts': [0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 1, 2, 0, 1, 0, 0, 0, 0, 0]},
        { 'field': 'int', 'counts': [0, 1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0]},
        { 'field': 'ush', 'counts': [0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 3, 0, 0]}
    )
)
assert(ok)

ok = VisualStatisticsTest('histogram outside bin range',
    (
        ArrayFromList(vtkDoubleArray, 'dbl', [-8.5, 0, 2.5, 4.25, math.nan, 10]),
        ArrayFromList(vtkIntArray, 'int', [-4, 0, 3, 5, 3, 48]),
        ArrayFromList(vtkUnsignedShortArray, 'ush', [1, 0, 3, 5, 5, 6]),
    ),
    { 'dbl': [-7,7], 'int': [-2, 30], 'ush': [0, 5] },
    (
        { 'field': 'dbl', 'counts': [1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1]},
        { 'field': 'int', 'counts': [1, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0]},
        { 'field': 'ush', 'counts': [0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 1, 0]}
    )
)
assert(ok)
