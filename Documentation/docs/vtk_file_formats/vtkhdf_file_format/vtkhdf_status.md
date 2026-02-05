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
| vtkOverlappingAMR               | partial [1](https://gitlab.kitware.com/vtk/vtk/-/issues/19926)[2](https://gitlab.kitware.com/vtk/vtk/-/issues/19927)  |
| vtkPolyData                     | specified       |
| vtkRectilinearGrid              | [not specified](https://gitlab.kitware.com/vtk/vtk/-/issues/19379) |
| vtkStructuredGrid               | [not specified](https://discourse.vtk.org/t/vtkhdf-vtkstructuredgrid-support/15920) |
| vtkUnstructuredGrid             | specified |

### Additional Features

Here is the list of supported feature which doesn't depends on a VTK data type:

| Features                        | Status          |
| ------------------------------- | --------------- |
| Static¹                         | supported       |
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
| Compression                     | supported           | supported      |
| Static¹                         | [partially supported²](https://gitlab.kitware.com/vtk/vtk/-/issues/19746) | [not supported](https://gitlab.kitware.com/vtk/vtk/-/issues/19888) |
| Temporal                        | supported           | supported      |

¹: ability to reuse data instead of duplicating it to save disk space and performance.
²: missing for vtkHyperTreeGrid.
