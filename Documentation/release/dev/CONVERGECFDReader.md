## New CONVERGE CFD post reader

Added a reader for CONVERGE CFD post files. This reader reads files containing
meshes, surfaces, and parcels. Each stream in a file is read as a top-level
block and meshes, surfaces, and parcels are datasets under each stream block.

Cell data arrays associated with mesh cells can be individually
selected for reading using the CellArrayStatus API.

Point data arrays associated with parcels can be individually selected
for reading using the ParcelArrayStatus API.

Time series are supported. The reader assumes a time series is defined
in a sequence of files that follow the naming convention

`<prefix><zero-padded index>[_][<time>].h5`

where the prefix is determined from the FileName property passed to
the reader. The underscore and time elements are optional. The time
value associated with each file is read from metadata in the file.

Parallel data loading is not supported.
