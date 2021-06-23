## Enabled thresholding points by vector array component or magnitude

`vtkThresholdPoints` now has a `InputArrayComponent` member variable that enables selection of a component in the active data array to use for thresholding. If the component value is set to a number larger than or equal to the number of components in the input array to process, the magnitude of each array tuple will be used for thresholding instead of an individual component value.
