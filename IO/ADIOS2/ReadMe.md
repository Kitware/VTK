# IO Reader ADIOS2

## Goal

Provide readers to data produced by the Adaptable Input / Output System version 2, [ADIOS2](https://adios2.readthedocs.io/en/latest/). Start with bp files/streams with a vtk.xml attribute schema the reuses the [VTK XML file formats schemas](https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf)

## Structure


- **vtkADIOS2ReaderDriver** : only public user-interfacing class


- **ADIOS2Helper** : collection of helper functions used privately in *.cxx


- **ADIOS2Schema** : abstract base class for supported schema

    - ADIOS2xmlVTI : ImageData VTK xml format
