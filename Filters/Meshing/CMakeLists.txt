set(classes
  vtkAnnularBinIterator
  vtkGeneralizedSurfaceNets3D
  vtkShellBinIterator
  vtkVoronoi2D
  vtkVoronoi3D
  vtkVoronoiHull
  vtkVoronoiTile
  )

set(template_classes
  vtkVoronoiCore
  vtkVoronoiCore2D
  vtkVoronoiCore3D
  )

set(nowrap_classes
  )

set(headers
  )

set(nowrap_headers
  )

set(templates
  )

set(private_templates
  )

include(vtkTypeLists)

vtk_module_add_module(VTK::FiltersMeshing
  CLASSES           ${classes}
  NOWRAP_CLASSES    ${nowrap_classes}
  TEMPLATES         ${templates}
  TEMPLATE_CLASSES  ${template_classes}
  HEADERS           ${headers}
  SOURCES           ${sources}
  NOWRAP_HEADERS    ${nowrap_headers}
  PRIVATE_TEMPLATES ${private_templates})
vtk_add_test_mangling(VTK::FiltersMeshing)
