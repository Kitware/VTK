## Rewrite reader for EnSight Gold files

The EnSight Gold reader is in the process of being rewritten. This adds two classes, `vtkEnSightGoldCombinedReader` and `vtkEnSightSOSGoldReader`, to handle EnSight gold casefiles and SOS files.
With the exception of being able to run in parallel (that will be coming in a future MR), this reader should have all of the functionality of the old reader, as well as supporting some things that the old reader did not. Notable differences between this reader and the old reader:

- No complicated class hierarchy. Hopefully this will be easier to maintain and figure out where problems are.
- There is an internal class, EnSightFile that handles all file operations and will handle ASCII, C binary, and Fortran binary. That makes it so the EnSightDataSet internal class doesn't have to care about what type of file is being read and can just focus on handling the logic around the different types of data (there are some rare cases where the way ASCII and binary are formatted in inconsistent ways, but for the most part EnSightDataSet doesn't need to care about file type).
- You can select which parts to load. By default, it will load all parts, but similar to loading selected arrays, you can load selected parts.
- Static geometry is cached.
- Outputs `vtkPartitionedDataSetCollection`
