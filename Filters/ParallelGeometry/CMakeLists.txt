vtk_object_factory_declare(
  BASE      vtkConnectivityFilter
  OVERRIDE  vtkPConnectivityFilter)
vtk_object_factory_declare(
  BASE      vtkDistributedDataFilter
  OVERRIDE  vtkPDistributedDataFilter)
vtk_object_factory_declare(
  BASE      vtkUnstructuredGridGhostCellsGenerator
  OVERRIDE  vtkPUnstructuredGridGhostCellsGenerator)

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSPARALLELGEOMETRY_EXPORT")

set(classes
  vtkPConnectivityFilter
  vtkPDataSetGhostGenerator
  vtkPDistributedDataFilter
  vtkPStructuredGridConnectivity
  vtkPStructuredGridGhostDataGenerator
  vtkPUniformGridGhostDataGenerator
  vtkPUnstructuredGridGhostCellsGenerator)

vtk_module_add_module(VTK::FiltersParallelGeometry
  CLASSES ${classes}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header})
