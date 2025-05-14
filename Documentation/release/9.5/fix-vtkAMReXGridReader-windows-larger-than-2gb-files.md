## Fix vtkAMReXGridReader when reading larger than 2GB files in Windows

You can now open grid files larger than 2GB on Windows using the `vtkAMReXGridReader`.
Previously, a bug in the reader prevented it from loading arrays stored beyond the 2GB offset in the file.
