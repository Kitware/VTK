# IO Reader ADIOS2

## Goal

Provide readers to data produced by the Adaptable Input / Output System version 2, [ADIOS2](https://adios2.readthedocs.io/en/latest/). Start with bp files/streams with a vtk.xml attribute schema the reuses the [VTK XML file formats schemas](https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf)

Currently used on Paraview ADIOS2ReaderMultiblock plugin development

Extensions:

* .h = header declaration
* .inl = generic inline template implementations
* .txx = specialized template implementations
* .cxx = implementation

## Structure


- **vtkADIOS2ReaderMultiBlock .h/.cxx** : only public user-interfacing class to Paraview plugin. Contains a member of type ADIOS2SchemaManager


- **ADIOS2DataArray .h/.cxx** : wrapper around vtkDataArray with adios2-related relevant information


- **ADIOS2Helper .h/.inl/.txx/.cxx** : collection of helper functions used privately in *.cxx


- **ADIOS2Types.h** : header only types definitions including MACROS


- **ADIOS2SchemaManager** : reusable class that manages a reader that is a derived type of ADIOS2Schema


- **schema/ADIOS2Schema .h/.txx/.cxx** : abstract base class for supported schema

    - schema/xml_vtk/ADIOS2xmlVTI : ImageData VTK xml format
    - schema/xml_vtk/ADIOS2xmlVTU : TODO: Unstructured VTK xml format
