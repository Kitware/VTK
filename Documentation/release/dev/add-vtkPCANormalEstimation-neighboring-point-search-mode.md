## Add neighboring point search modes for vtkPCANormalEstimation

`vtkPCANormalEstimation` now provides 2 modes to configure how the filter selects the neighbor points used to calculate the PCA:
 - KNN: By default, K points are selected regardless of their location relative to the sampled point. The radius can also be set to ensure that a sufficiently large neighborhood is taken into account, if not the next approach is performed.
 - RADIUS: A second approach is to select neighboring points inside a radius, only the neighborhood of the sampled is considered. If K is also set, the number of points found inside the radius must be larger than K, if not the first approach is performed.
