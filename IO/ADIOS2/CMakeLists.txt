vtk_module_find_package(
  PACKAGE ADIOS2
  VERSION 2.4)
if (VTK_USE_MPI AND NOT ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built with MPI requires ADIOS2 built with MPI")
elseif(NOT VTK_USE_MPI AND ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built without MPI requires ADIOS2 built without MPI")
endif()

# Initial empty set of files
set(classes_core)
set(private_classes_core)
set(private_headers_core)
set(private_templates_core)

# ADIOS2 Core Image Reader
set(classes_core vtkADIOS2CoreImageReader)
set(private_classes_core Core/vtkADIOS2CoreArraySelection)
set(private_headers_core Core/vtkADIOS2CoreTypeTraits.h)
set(private_templates_core)
set(vtk_io_adios2_core_enabled TRUE CACHE INTERNAL "" FORCE)

if (vtk_io_adios2_core_enabled)
  list(APPEND classes   ${classes_core})
  list(APPEND private_classes   ${private_classes_core})
  list(APPEND private_headers   ${private_headers_core})
  list(APPEND private_templates ${private_templates_core})
endif()

set(classes_vtx vtkADIOS2VTXReader)
set(private_classes_vtx
  VTX/VTXSchemaManager
  VTX/common/VTXDataArray
  VTX/common/VTXHelper
  VTX/schema/VTXSchema
  VTX/schema/vtk/VTXvtkBase
  VTX/schema/vtk/VTXvtkVTI
  VTX/schema/vtk/VTXvtkVTU)
set(private_headers_vtx VTX/common/VTXTypes.h)
set(private_templates_vtx
  VTX/common/VTXHelper.txx
  VTX/schema/VTXSchema.txx
  VTX/schema/vtk/VTXvtkVTI.txx
  VTX/schema/vtk/VTXvtkVTU.txx)

if (VTK_USE_MPI)
  set(vtk_io_adios2_vtx_enabled TRUE CACHE INTERNAL "" FORCE)
else ()
  set(vtk_io_adios2_vtx_enabled FALSE CACHE INTERNAL "" FORCE)
endif()

if (vtk_io_adios2_vtx_enabled)
  list(APPEND classes   ${classes_vtx})
  list(APPEND private_classes   ${private_classes_vtx})
  list(APPEND private_headers   ${private_headers_vtx})
  list(APPEND private_templates ${private_templates_vtx})
endif ()

vtk_module_add_module(VTK::IOADIOS2
  CLASSES   ${classes}
  PRIVATE_CLASSES   ${private_classes}
  PRIVATE_HEADERS   ${private_headers}
  PRIVATE_TEMPLATES ${private_templates})
vtk_module_link(VTK::IOADIOS2 PRIVATE adios2::adios2)

if (ADIOS2_HAVE_MPI)
  vtk_module_definitions(VTK::IOADIOS2 PRIVATE IOADIOS2_HAVE_MPI)
endif ()
