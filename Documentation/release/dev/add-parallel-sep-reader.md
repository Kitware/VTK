# Add vtkPSEPReader to read SEP Reader in parallel

The `vtkPSEPReader` class has been added to read the header of SEP files only in rank 0 and then broadcast it to all ranks.
The extents of the file are then shared among ranks.
