vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE ADIOS2
  VERSION 2.4)
if (TARGET VTK::ParallelMPI AND NOT ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built with MPI requires ADIOS2 built with MPI")
elseif (NOT TARGET VTK::ParallelMPI AND ADIOS2_HAVE_MPI)
  message(FATAL_ERROR "VTK built without MPI requires ADIOS2 built without MPI")
endif ()

# Initial empty set of files
set(classes_core)
set(private_headers_core)
set(private_templates_core)

# ADIOS2 Core Image Reader
set(classes_core vtkADIOS2CoreImageReader)
set(private_headers_core Core/vtkADIOS2CoreTypeTraits.h)
set(private_templates_core)

list(APPEND classes   ${classes_core})
list(APPEND private_headers   ${private_headers_core})
list(APPEND private_templates ${private_templates_core})

# Build VTX Schema for Parallel
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

list(APPEND classes   ${classes_vtx})
list(APPEND private_classes   ${private_classes_vtx})
list(APPEND private_headers   ${private_headers_vtx})
list(APPEND private_templates ${private_templates_vtx})

vtk_module_add_module(VTK::IOADIOS2
  CLASSES   ${classes}
  PRIVATE_CLASSES   ${private_classes}
  PRIVATE_HEADERS   ${private_headers}
  PRIVATE_TEMPLATES ${private_templates})
vtk_module_link(VTK::IOADIOS2 NO_KIT_EXPORT_IF_SHARED PRIVATE adios2::adios2)

set(adios_have_bp5_random_access 0)
if (ADIOS2_VERSION VERSION_GREATER_EQUAL "2.8.0")
  set(adios_have_bp5_random_access 1)
endif ()
set_property(SOURCE VTX/VTXSchemaManager.cxx vtkADIOS2CoreImageReader.cxx APPEND
  PROPERTY
    COMPILE_DEFINITIONS "IOADIOS2_BP5_RANDOM_ACCESS=$<BOOL:${adios_have_bp5_random_access}>")

vtk_add_test_mangling(VTK::IOADIOS2)
