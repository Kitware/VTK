vtk_object_factory_declare(
  BASE      vtkAggregateDataSetFilter
  OVERRIDE  vtkDIYAggregateDataSetFilter)
vtk_object_factory_declare(
  BASE      vtkResampleToImage
  OVERRIDE  vtkPResampleToImage)
vtk_object_factory_declare(
  BASE      vtkResampleWithDataSet
  OVERRIDE  vtkPResampleWithDataSet)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSPARALLELDIY2_EXPORT")

set(classes
  vtkDIYAggregateDataSetFilter
  vtkPResampleToImage
  vtkPResampleWithDataSet)

vtk_module_add_module(VTK::FiltersParallelDIY2
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})

vtk_module_definitions(VTK::FiltersParallelDIY2
  PRIVATE DIY_NO_THREADS)
