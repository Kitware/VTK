# Roadmap

This document specifies the features currently supported by the VTK implementation of the VTKHDF file format, `vtkHDFReader` and `vtkHDFWriter`.
The following list only showcases complete and planned developments.

## VTKHDF File Format Specifications Status

### Data Types

VTKHDF File Format should support at some point every data type of VTK (e.g. `vtkTable`, `vtkMolecule`,...).

The following list shows the VTK data types currently supported, at least partially.

| VTK Data Type                   | Status          |
| ------------------------------- | --------------- |
| vtkPolyData                     | supported       |
| vtkImageData                    | supported       |
| vtkUnstructuredGrid             | [partially supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19237) |
| vtkOverlappingAMR               | supported       |
| vtkHyperTreeGrid                | supported       |
| vtkRectilinearGrid              | [not implemented](https://gitlab.kitware.com/vtk/vtk/-/issues/19379) |
| vtkCellGrid                     | [not implemented](https://discourse.vtk.org/t/vtkhdf-roadmap/13257/19) |
| vtkPartitionedDataSet           | supported       |
| vtkMultiBlockDataSet            | supported       |
| vtkPartitionedDataSetCollection | supported       |

### Additional Features

Here is the list of supported feature which doesn't depends on a VTK data type:

| Features                        | Status          |
| ------------------------------- | --------------- |
| Temporal                        | supported       |
| Static                          | supported       |

## Implementation Status in VTK

### Data Types

| VTK Data Type                   | vtkHDFReader    |vtkHDFWriter    |
| ------------------------------- | --------------- |--------------- |
| vtkPolyData                     | supported       | supported      |
| vtkImageData                    | supported       | not suported   |
| vtkUnstructuredGrid             | supported       | supported      |
| vtkOverlappingAMR               | supported       | not suported   |
| vtkHyperTreeGrid                | supported       | not suported   |
| vtkPartitionedDataSet           | supported       | supported      |
| vtkPartitionedDataSetCollection | supported       | supported      |
| vtkMultiBlockDataSet            | supported       | supported      |

### Additional Features

| Features                        | vtkHDFReader    |vtkHDFWriter    |
| ------------------------------- | --------------- |--------------- |
| Temporal                        | supported       |supported       |
| Static                          | [partially supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19746)       |not supported |
| Compression                     | supported       |supported       |
