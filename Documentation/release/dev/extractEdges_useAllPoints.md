## Added UseAllPoints Option to vtkExtractEdges

When dealing with the situation where all of the input points will
exist in the output result, the use of a locator is redundant and
on large meshes can significantly increase the execution time.

On a 10 million cell surface mesh, the default locator implementation
took about 7 mins on a 2019 6 core macbook pro.

A new mode (UseAllPoints) has been added to indicate that all of the
points in the input should exist in the output.  When set the filter
will use a non-Locator based approach.
