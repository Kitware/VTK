vtk_module_find_package(
  PACKAGE ADIOS1
  VERSION 1.11)
vtk_module_find_package(
  PACKAGE Threads)

set(classes
  ADIOSAttribute
  ADIOSDefs
  ADIOSReader
  ADIOSScalar
  ADIOSUtilities
  ADIOSVarInfo
  ADIOSWriter
  vtkADIOSDirTree
  vtkADIOSReader
  vtkADIOSUtilities
  vtkADIOSWriter)

vtk_module_add_module(VTK::IOADIOS
  CLASSES ${classes})
vtk_module_link(VTK::IOADIOS
  PRIVATE
    adios1::adios
    Threads::Threads)
