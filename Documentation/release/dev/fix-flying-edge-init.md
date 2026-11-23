# Fix an initialization issue in flying edge filters

Fix an issue in flying edge filter where an array would be left uninitialized,
causing potential crashes. The filters concerned are:
 - vtkDiscreteFlyingEdgesClipper2D
 - vtkDiscreteFlyingEdges2D
 - vtkFlyingEdgesPlaneCutter
 - vtkFlyingEdges2D
