# ColorTransferFunction - AddRGBPoints in Bulk

Adding a higher performance rgb point routine. The SortAndUpdateRange call
after every point adds significant overhead when adding a large number of
points. If we add points in bulk, then run the sort, we end up at the same
resulting state with much better performance.
