# Tube Bender

vtkTubeBender is designed to generate better tube paths for vtkTubeFilter.

For points with very sharp inflection point angles, the radius used to determine
where surface points are placed becomes more parallel, instead of perpendicular,
to the tube path. This causes the tube to become more oblong, as the
cross-sectional height and width become increasingly disparate.

This filter inserts new points along the tube path near acute angles to reduce
the amount the point normals will change from point to point. This reduces the
cross-sectional height and width variations from over 95% to less than 30%.
This gives an impression of a constant diameter tube with nice acute angle bends
without adding too many new points (and therefore faces).
