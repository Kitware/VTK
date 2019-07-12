# VTK::IOADIOS2 Module

## Goal

Provide readers to data produced by the Adaptable Input Output System version 2, [ADIOS2](https://adios2.readthedocs.io/en/latest/).

Currently used on Paraview Application Server Manager development

Extensions:

* .h = header declaration
* .inl = generic inline template implementations
* .txx = specialized template implementations
* .cxx = implementation

##Public VTK classes:

- **vtkVARMultiBlock .h/.cxx** : multiblock reader for ImageData and UnstructuredData types using VTK ADIOS2 Readers (VAR) implementation developed at Oak Ridge National Laboratory (ORNL). Reads bp files/streams with a vtk.xml attribute schema the reuses the [VTK XML file formats schemas](https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf)


## **VAR: VTK ADIOS2 READERS**

Developed at Oak Ridge National Laboratory

- **common/VARDataArray .h/.cxx** : wrapper around vtkDataArray with adios2-related relevant information


- **common/VARHelper .h/.inl/.txx/.cxx** : collection of helper functions used privately in *.cxx


- **common/VARTypes.h** : header only types definitions including MACROS


- **VARSchemaManager** : reusable class that manages a reader that is a derived type of VARSchema


- **schema/VARSchema .h/.txx/.cxx** : abstract base class for supported schema
    - schema/vtk/VARvtkBase : Base class for VTK formats
    - schema/vtk/VARvtkVTI : ImageData VTK format
    - schema/vtk/VARvtkVTU : Unstructured VTK format
