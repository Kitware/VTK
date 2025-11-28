# VTKHDF: Implementation Status

This document specifies the features currently supported by the VTK implementation of the VTKHDF file format, `vtkHDFReader` and `vtkHDFWriter`.
The following list only showcases complete and planned developments.

## File Format Specifications Status

### Data Types

The following list shows the VTK data types currently specified, at least partially.
Any data type not listed here is also not yet specified, the one that are listed has not specified
have ongoing discussions and may be specified when possible.

| VTK Data Type                   | Status          |
| ------------------------------- | --------------- |
| vtkCellGrid                     | [not specified](https://discourse.vtk.org/t/vtkhdf-roadmap/13257/19) |
| vtkHyperTreeGrid                | specified       |
| vtkImageData                    | specified       |
| vtkMultiBlockDataSet            | specified       |
| vtkPartitionedDataSet           | specified       |
| vtkPartitionedDataSetCollection | specified       |
| vtkOverlappingAMR               | specified       |
| vtkPolyData                     | specified       |
| vtkRectilinearGrid              | [not specified](https://gitlab.kitware.com/vtk/vtk/-/issues/19379) |
| vtkStructuredGrid               | [not specified](https://discourse.vtk.org/t/vtkhdf-vtkstructuredgrid-support/15920) |
| vtkUnstructuredGrid             | [partially specified](https://gitlab.kitware.com/vtk/vtk/-/issues/19237) |

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
| vtkHyperTreeGrid                | supported       | not supported  |
| vtkImageData                    | supported       | not supported  |
| vtkMultiBlockDataSet            | supported       | supported      |
| vtkPartitionedDataSet           | supported       | supported      |
| vtkPartitionedDataSetCollection | supported       | supported      |
| vtkOverlappingAMR               | supported       | not supported  |
| vtkPolyData                     | supported       | supported      |
| vtkUnstructuredGrid             | supported       | supported      |

### Additional Features

| Features                        | vtkHDFReader    |vtkHDFWriter    |
| ------------------------------- | --------------- |--------------- |
| Compression                     | supported       |supported       |
| Static*                         | [partially supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19746)       |not supported |
| Temporal                        | supported       |supported       |

*: ability to reuse dataset from another place to save disk space and performance.
