## Add Gaussian integration strategy

The `vtkIntegrateAttributes` filter now accepts different integration strategies. The original way of integration is now called Linear and the Gaussian Integration strategy has been added. The Gaussian Integration handles higher order cells and n-linear degenerate cells like non planar quads or hexahedron with non planar faces. Also, the Gaussian integration is not affected by a different point ordering as long as the constraints specified for each cell are maintained.
