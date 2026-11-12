# vtkTable modified time includes its columns

`vtkTable::GetMTime()` now includes the modified time of the table's row data,
and therefore of its columns, like `vtkDataSet` does for its point and cell
data. Modifying a column now modifies the table, so filters that compare the
modified time of an input table detect changes to its values.
