# Make temporal cache deep copy by default

Previously, the `vtkTemporalDataSetCache` filter had trouble dealing with changes to data upstream
of the pipeline. For example, the EnSight 6 readers' point caches were always reallocated in place.
This interferes with things that might store pointers to certain data downstream (like the temporal
cache).

This fix makes the temporal cache deep copy data by default. It also makes the EnSight6 readers more
robust by renewing the internal point cache every time the points are read from the file.
