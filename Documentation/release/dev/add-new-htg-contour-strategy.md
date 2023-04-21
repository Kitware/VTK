## Add new HTG contour strategy

HTG contour now have 2 contour strategy in the 3D case.
The former one is called VTK_HTG_CONTOUR_FAST and corresponds to the original behavior.
The second one is called VTK_HTG_CONTOUR_QUALITY and allow better contour results in some cases
(when generated dual cells used for contouring appears to be concave).
Note that this new strategy is much more slower than the first one.
