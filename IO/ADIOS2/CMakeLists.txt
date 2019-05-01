vtk_module_find_package(
  PACKAGE ADIOS2
  )

set(classes
  vtkADIOS2Reader
  )

set(sources
  vtkADIOS2Reader.cxx
  ADIOS2Helper.cxx ADIOS2Helper.txx
  ADIOS2Schema.cxx ADIOS2Schema.txx
  xml_vtk/ADIOS2xmlVTI.cxx
  xml_vtk/ADIOS2xmlVTK.cxx
)

set(private_headers
  ADIOS2Helper.h
  ADIOS2Schema.h
  ADIOS2Types.h
  xml_vtk/ADIOS2xmlVTI.h
  xml_vtk/ADIOS2xmlVTK.h
)

vtk_module_add_module(VTK::IOADIOS2
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)

vtk_module_link(VTK::IOADIOS2
  PRIVATE
    adios2::adios2
)
