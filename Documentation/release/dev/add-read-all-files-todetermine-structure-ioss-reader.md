## vtkIOSSReader: Add ReadAllFilesToDetermineStructure flag

vtkIOSSReader has a flag called ReadAllFilesToDetermineStructure. This flag is used to determine the structure of the
mesh. If this flag is set to true, then all the files are read to determine the structure of the mesh. If this flag is
set to false, then only the first file is read to determine the structure of the mesh. This flag is set to true by
default, to ensure that the structure of the mesh is determined correctly. However, if you know that the structure of
the mesh is the same in all the files, then you can set this flag to false to improve the performance of the reader.
