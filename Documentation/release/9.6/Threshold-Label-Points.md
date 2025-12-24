## VTK can now segment a scalar field into discrete intervals

VTK can now generate discrete segmentation labels from a continuous scalar
field.  The newly added filter vtkThresholdScalars segments the scalars
through the definition of one or more [min,max,labelId) scalar intervals.
