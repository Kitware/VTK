# vtkOpenFOAMReader: Multithreaded Reading of case files

`vtkOpenFOAMReader` now supports multithreaded reading of case files. This feature is on by default, and it can be
disabled using the`SetSequentialProcessing(true)`. It can be useful to be enabled when reading large case files that
are stored on a network drive. If the case file is stored on a local drive, it is recommended to keep this feature off.
Also `ReadAllFilesToDetermineStructure` has been added, which enables reading only the proc 0 directory to determine the
structure of the case file, and broadcast it to all processors. This is off by default, because there is not such
guarantee that the proc 0 directory contains all the necessary information to determine the structure of the case file.
Finally, several performance improvements have been made to the reader.
