# Roadmap

This document specifies the features currently supported by the VTK implementation of the VTKHDF file format, `vtkHDFReader` and `vtkHDFWriter`.
The following list only showcases complete and planned developments.

## VTKHDF File Format Specifications Status

### Data Types

VTKHDF File Format should support at some point every data type of VTK (e.g. `vtkTable`, `vtkMolecule`,...).

The following list shows the VTK data types currently supported, at least partially.

| VTK Data Type                   | Status          |
| ------------------------------- | --------------- |
| vtkCellGrid                     | [not implemented](https://discourse.vtk.org/t/vtkhdf-roadmap/13257/19) |
| vtkHyperTreeGrid                | supported       |
| vtkImageData                    | supported       |
| vtkMultiBlockDataSet            | supported       |
| vtkPartitionedDataSet           | supported       |
| vtkPartitionedDataSetCollection | supported       |
| vtkOverlappingAMR               | supported       |
| vtkPolyData                     | supported       |
| vtkRectilinearGrid              | [not implemented](https://gitlab.kitware.com/vtk/vtk/-/issues/19379) |
| vtkStructuredGrid               | [not implemented](https://discourse.vtk.org/t/vtkhdf-vtkstructuredgrid-support/15920) |
| vtkUnstructuredGrid             | [partially supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19237) |

### Additional Features

Here is the list of supported feature which doesn't depends on a VTK data type:

| Features                        | Status          |
| ------------------------------- | --------------- |
| Static*                         | supported       |
| Temporal                        | supported       |

## Implementation Status in VTK

### Data Types

| VTK Data Type                   | vtkHDFReader    |vtkHDFWriter    |
| ------------------------------- | --------------- |--------------- |
| vtkHyperTreeGrid                | supported       | not suported   |
| vtkImageData                    | supported       | not suported   |
| vtkMultiBlockDataSet            | supported       | supported      |
| vtkPartitionedDataSet           | supported       | supported      |
| vtkPartitionedDataSetCollection | supported       | supported      |
| vtkOverlappingAMR               | supported       | not suported   |
| vtkPolyData                     | supported       | supported      |
| vtkStructuredGrid               | not supported   | not supported  |
| vtkUnstructuredGrid             | supported       | supported      |

### Additional Features

| Features                        | vtkHDFReader    |vtkHDFWriter    |
| ------------------------------- | --------------- |--------------- |
| Compression                     | supported       |supported       |
| Static*                          | [partially supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19746)       |not supported |
| Temporal                        | supported       |supported       |

*: ability to reuse dataset from another place to save disk space and performance.
