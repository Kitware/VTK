# vtkAlignImageDataSetFilter - filter to align origins

When dealing with a collection of image datasets, either in a composite
dataset or in a distributed environment, it is not uncommon to have each
dataset have its own unique origin such that the extents for each start at 0.
However, if the images are parts of a whole, then several filters like
vtkExtractVOI that simply use extents fail to execute correctly. Such
filters require that all parts use the same global origin and set local
extents accordingly. A new filter, `vtkAlignImageDataSetFilter`,
can be used to align such image datasets. Essentially, this filter ensures
all image datasets have the same origin and each blocks extents are set
relative to that origin. This requires that all images have the same spacing.
