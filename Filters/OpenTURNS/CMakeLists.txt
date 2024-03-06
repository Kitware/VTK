vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE OpenTURNS
  VERSION 1.12)

set(classes
  vtkOTDensityMap
  vtkOTFilter
  vtkOTKernelSmoothing
  vtkOTScatterPlotMatrix
  vtkOTUtilities)

vtk_object_factory_declare(
  BASE vtkScatterPlotMatrix
  OVERRIDE vtkOTScatterPlotMatrix)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSOPENTURNS_EXPORT")

vtk_module_add_module(VTK::FiltersOpenTURNS
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

vtk_module_link(VTK::FiltersOpenTURNS
  PRIVATE
    ${OPENTURNS_LIBRARY})
vtk_add_test_mangling(VTK::FiltersOpenTURNS)
