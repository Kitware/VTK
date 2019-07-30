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

- **vtkADIOS2ImageCoreReader.h/.cxx** : a generic multiblock reader for image data developed at Kitware Inc. It will use existing arrays to populate dimension of the image, adding timesteps info, point and cell data accordingly. No predefined schema is needed. It can work in serial or MPI mode.

- **vtkADIOS2VTXReader .h/.cxx** : multiblock reader for ImageData and UnstructuredData types using VTK ADIOS2 Readers (VTX) implementation developed at Oak Ridge National Laboratory (ORNL). Reads bp files/streams with a vtk.xml attribute schema the reuses the [VTK XML file formats schemas](https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf). For more comprehensive documentation refer [to this section in the ADIOS2 User Guide.](https://adios2.readthedocs.io/en/latest/ecosystem/visualization.html)

## **Core: VTK ADIOS2 CORE READERS**

Developed at Kitware Inc
- **vtkADIOS2CoreArraySelection.h/.cxx**: Array selection for client and server communication
- **vtkADIOS2CoreTypeTraits.h** TypeTraits from adios2 type to vtk type

## **VTX: VTK ADIOS2 READERS**

Developed at Oak Ridge National Laboratory

- **common/VTXDataArray .h/.cxx** : wrapper around vtkDataArray with adios2-related relevant information


- **common/VTXHelper .h/.inl/.txx/.cxx** : collection of helper functions used privately in *.cxx


- **common/VTXTypes.h** : header only types definitions including MACROS


- **VTXSchemaManager** : reusable class that manages a reader that is a derived type of VTXSchema


- **schema/VTXSchema .h/.txx/.cxx** : abstract base class for supported schema
    - schema/vtk/VTXvtkBase : Base class for VTK formats
    - schema/vtk/VTXvtkVTI : ImageData VTK format
    - schema/vtk/VTXvtkVTU : Unstructured VTK format
